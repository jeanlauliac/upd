#!/usr/bin/env node
/* @flow */

'use strict';

const cli = require('./lib/cli');
const fs = require('fs');
const path = require('path');
const reporting = require('./lib/reporting');
const writeNodeDepFile = require('./lib/writeNodeDepFile');

type Manifest = {
  description: string,
  namespace: Array<string>,
  includes: Array<string>,
  commands: {
    [string]: {
      description: string,
    },
  },
  options: Array<{
    default: string,
    description: string,
    name: string,
    only_for?: Array<string>,
    value_type: string,
    parse_function?: string,
  }>,
};

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
  writeNodeDepFile(depfilePath, targetCppPath);
});

function genSpec(manifest: Manifest) {
  const types = [];
  const commands = Object.entries(manifest.commands).map(([name, command]: [string, $FlowFixMe]) => {
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
      description: option.description,
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
      stream.write(`${type.name} parse_${type.name}(const std::string& str) {\n`);
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
  os << ansi_sgr({4, 34}, use_color) << "Usage:";
  os << ansi_sgr({}, use_color) << " " << program << " <command> [options]" << std::endl;
  os << "${spec.description}" << std::endl << std::endl;
  os << ansi_sgr({4, 34}, use_color) << "Commands:";
  os << ansi_sgr({}, use_color) << std::endl;
`);
  for (const command of spec.commands) {
    stream.write(`  os << " " << ansi_sgr(1, use_color) << "${rightPad(command.name, 12)}";\n`);
    stream.write(`  os << ansi_sgr({}, use_color) << "  ${command.description}" << std::endl;\n`);
  }
  stream.write(`  os << std::endl << ansi_sgr({4, 34}, use_color) << "General options:";
  ansi_sgr(os, {}, use_color) << std::endl;`);
  genOptionsListing(stream, '    ', spec.options.filter(opt => opt.onlyFor == null));
  stream.write(`}

void output_help(const std::string& program, const command& command, bool use_color, std::ostream& os) {
  os << ansi_sgr({4, 34}, use_color) << "Usage:"
     << ansi_sgr(0, use_color) << " " << program << " ";
  switch (command) {
`);
  for (const command of spec.commands) {
    const cmdOptions = spec.options.filter(opt => (
      opt.onlyFor != null &&
      opt.onlyFor.indexOf(command.name) >= 0
    ));
    stream.write(`  case command::${command.cppName}:\n`);
    stream.write(`    os << "${command.name}`);
    if (cmdOptions.length > 0) stream.write(` [options]`);
    stream.write(`" << std::endl;\n`);
    stream.write(`    os << "${command.description}" << std::endl`);
    if (cmdOptions.length > 0) stream.write(` << std::endl;\n`);
    else stream.write(`;\n`);
    if (cmdOptions.length > 0) {
      stream.write(`    os << ansi_sgr({4, 34}, use_color) << "Command options:";\n`);
      stream.write(`    os << ansi_sgr({}, use_color) << std::endl;\n`);
      genOptionsListing(stream, '    ', cmdOptions);
    }
    stream.write(`    break;\n`);
  }
  stream.write('  }\n}\n\n');
  genNamespaceClose(spec.namespace, stream);
}

function genOptionsListing(stream, indent, options) {
  for (const option of options) {
    stream.write(`${indent}os << ansi_sgr(1, use_color) << " --${rightPad(option.name, 12)}";\n`);
    stream.write(`${indent}os << ansi_sgr({}, use_color) << "  ${option.description}" << std::endl;\n`);
  }
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
  invalid_${type.name}_error(const std::string& value_): value(value_) {}
  const std::string value;
};

${type.name} parse_${type.name}(const std::string& str);
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
    if (option.defaultValue != null) {
      stream.write(`    ${option.cppName}(${option.defaultValue})`);
      first = false;
    }
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
  unexpected_argument_error(const std::string& arg_): arg(arg_) {}
  const std::string arg;
};

struct option_requires_argument_error {
  option_requires_argument_error(const std::string& option_): option(option_) {}
  const std::string option;
};

struct missing_command_error {
  missing_command_error(const std::string& program_name_):
    program_name(program_name_) {}
  const std::string program_name;
};

struct unavailable_option_for_command_error {};

options parse_options(const char* const argv[]);
void output_help(const std::string& program, bool use_color, std::ostream& os);
void output_help(const std::string& program, const command& command, bool use_color, std::ostream& os);

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
