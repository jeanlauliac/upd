#!/usr/bin/env node

'use strict';

const cli = require('./lib/cli');
const fs = require('fs');
const path = require('path');
const reporting = require('./lib/reporting');

cli(function () {
  if (process.argv.length < 6) {
    reporting.fatalError(1, 'not enough arguments');
    return;
  }
  const targetCppPath = process.argv[2];
  const targetHppPath = process.argv[3];
  const depfilePath = process.argv[4];
  const sourcePath = process.argv[5];
  const content = fs.readFileSync(sourcePath, 'utf8');
  const spec = genSpec(JSON.parse(content));
  const targetCppStream = fs.createWriteStream(targetCppPath);
  const relativeHppPath =
    path.relative(path.dirname(targetCppPath), targetHppPath);
  genCliCppParser(spec, targetCppStream, relativeHppPath);
  targetCppStream.end();
  const targetHppStream = fs.createWriteStream(targetHppPath);
  const relativeSourceDirPath = path.relative(
    path.dirname(targetHppPath),
    path.dirname(sourcePath)
  );
  genCliHppParser(spec, relativeSourceDirPath, targetHppStream);
  targetHppStream.end();
  const depfile = fs.createWriteStream(depfilePath);
  const modulePaths = Object.values(require.cache)
    .filter(module => !/\/node_modules\//.test(module.filename))
    .map(module => module.filename)
    .join(' \\\n  ');
  depfile.write(`${targetCppPath}: ${modulePaths}\n`);
  depfile.end();
});

function genSpec(manifest) {
  const types = [];
  const commands = Object.entries(manifest.commands).map(([name, command]) => {
    return {
      name,
      cppName: cppNameOf(name),
      description: command.description,
    };
  });
  types.push({name: 'command', spec: {enum_of: commands.map(x => ({
    cliName: x.name,
    cppName: x.cppName,
  })).sort((a, b) => a.cppName > b.cppName ? 1 : -1)}});
  const options = manifest.options.map(option => {
    const type = option.value_type || 'bool';
    const cppName = option.name.replace(/-/, '_');
    let valueType, defaultValue;
    if (typeof type === 'object') {
      if (type.enum_of != null) {
        const name = cppName;
        valueType = name;
        defaultValue = `${valueType}::${cppNameOf(option.default)}`;
        types.push({name, spec: {enum_of: type.enum_of.map(x => ({
          cliName: x,
          cppName: cppNameOf(x),
        })).sort()}});
      }
    } else {
      if (type === 'bool') {
        valueType = 'bool';
        defaultValue = option.default ? option.default.toString() : 'false';
      } else if (type === 'size_t') {
        valueType = 'size_t';
        defaultValue = option.default ? option.default.toString() : '0';
      }
    }
    if (valueType == null) {
      throw Error(`invalid type for option \`${option.name}\``);
    }
    return {
      cppName,
      name: option.name,
      defaultValue,
      valueType,
      onlyFor: option.only_for && option.only_for.map(c => cppNameOf(c)),
      parseFunction: option.parse_function,
    };
  }).sort((a, b) => a.cppName > b.cppName ? 1 : -1);
  return {
    includes: manifest.includes,
    commands,
    description: manifest.description,
    options,
    namespace: manifest.namespace,
    types: types.sort((a, b) => a.name > b.name ? 1 : -1),
  };
}

function cppNameOf(cliName) {
  const val = cliName.replace(/-/, '_');
  if (val === 'auto') {
    return val + '_';
  }
  return val;
}

function genCliCppParser(spec, stream, hppPath) {
  stream.write(`#include "${hppPath}"\n`);
  stream.write(`#include <iostream>\n\n`);
  stream.write(`#include <unordered_map>\n\n`);
  genNamespaceOpen(spec.namespace, stream);
  for (const type of spec.types) {
    if (type.spec.enum_of) {
      const mapName = `${type.name}_map`;
      stream.write(`static const std::unordered_map<std::string, ${type.name}> ${mapName} {\n`);
      for (const variant of type.spec.enum_of) {
        stream.write(`  { "${variant.cliName}", ${type.name}::${variant.cppName} },\n`);
      }
      stream.write(`};\n\n`);
      stream.write(`static ${type.name} parse_${type.name}(std::string str) {\n`);
      stream.write(`  auto iter = ${mapName}.find(str);
  if (iter != ${mapName}.cend()) return iter->second;
  throw invalid_${type.name}_error(str);
};

`);
      continue;
    }
    throw new Error('unknown type');
  }
  stream.write(`options parse_options(const char* const argv[]) {
  options result;
  bool reading_options = true;
  result.program_name = *argv;
  if (*(++argv) == nullptr) {
    throw missing_command_error(result.program_name);
  }
  result.command = parse_command(*argv);
  for (++argv; *argv != nullptr; ++argv) {
    const auto arg = std::string(*argv);
    if (!reading_options || arg.size() == 0 || arg[0] != '-') {
      result.rest_args.push_back(arg);
      continue;
    }
    if (arg.size() == 1 || arg[1] != '-') {
      throw unexpected_argument_error(arg);
    }
`);
  for (const option of spec.options) {
    stream.write(`    if (arg.compare(2, arg.size(), "${option.name}") == 0) {\n`);
    if (option.onlyFor != null) {
      stream.write(`      if (
        ${option.onlyFor.map(c => {
        return `result.command != command::${c}`;
      }).join(' &&\n        ')}
      ) {
        throw unavailable_option_for_command_error();
      }
`);
    }
    if (option.valueType === 'bool') {
      stream.write(`      result.${option.cppName} = true;\n`);
    } else {
      stream.write(`      ++argv;
      if (*argv == nullptr) {
        throw option_requires_argument_error("--${option.name}");
      }
`);
      const parseFunction = option.parseFunction || `parse_${option.valueType}`;
      stream.write(`      result.${option.cppName} = ${parseFunction}(*argv);\n`);
    }
    stream.write(`      continue;\n`);
    stream.write(`    }\n`);
  }
  stream.write(`    throw unexpected_argument_error(arg);
  }
  return result;
}

void output_help(const std::string& program, bool use_color, std::ostream& os) {
  os << "Usage: " << program << " <command> [options]" << std::endl;
  os << R"HELP(${spec.description}

Commands:
`);
  for (const command of spec.commands) {
    stream.write(`  ${rightPad(command.name, 12)}  ${command.description}\n`);
  }
  stream.write(`)HELP";
}
`);
  genNamespaceClose(spec.namespace, stream);
}

function rightPad(str, size) {
  return str + " ".repeat(Math.max(0, size - str.length));
}

function genCliHppParser(spec, relativeSourceDirPath, stream) {
  stream.write(`#pragma once

${spec.includes.map(x => `#include "${path.join(relativeSourceDirPath, x)}"`).join('\n')}
#include <string>
#include <vector>

`);
  genNamespaceOpen(spec.namespace, stream);
  for (const type of spec.types) {
    if (type.spec.enum_of) {
      stream.write(`enum class ${type.name} {\n`);
      for (const variant of type.spec.enum_of) {
        stream.write(`  ${variant.cppName},\n`);
      }
      stream.write(`};\n\n`);
      stream.write(`struct invalid_${type.name}_error {
  invalid_${type.name}_error(const std::string& value): value(value) {}
  const std::string value;
};

`);
      continue;
    }
    throw new Error('unknown type');
  }
  stream.write('struct options {\n');
  stream.write('  options():\n');
  let first = true;
  for (const option of spec.options) {
    if (!first) stream.write(',\n');
    stream.write(`    ${option.cppName}(${option.defaultValue})`);
    first = false;
  }
  stream.write(' {}\n\n');
  stream.write(`  command command;\n`);
  stream.write(`  std::vector<std::string> rest_args;\n`);
  stream.write(`  std::string program_name;\n`);
  for (const option of spec.options) {
    stream.write(`  ${option.valueType} ${option.cppName};\n`);
  }
  stream.write('};\n\n');
  stream.write(`struct unexpected_argument_error {
  unexpected_argument_error(const std::string& arg): arg(arg) {}
  const std::string arg;
};

struct option_requires_argument_error {
  option_requires_argument_error(const std::string& option): option(option) {}
  const std::string option;
};

struct missing_command_error {
  missing_command_error(const std::string& program_name):
    program_name(program_name) {}
  std::string program_name;
};

struct unavailable_option_for_command_error {};

options parse_options(const char* const argv[]);
void output_help(const std::string& program, bool use_color, std::ostream& os);

`);
  genNamespaceClose(spec.namespace, stream);
}

function genNamespaceOpen(namespace, stream) {
  for (const part of namespace) {
    stream.write(`namespace ${part} {\n\n`);
  }
}

function genNamespaceClose(namespace, stream) {
  for (const _ of namespace) {
    stream.write('}\n');
  }
}
