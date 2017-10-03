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
  genCliHppParser(spec, targetHppStream);
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
  const options = manifest.options.map(option => {
    const type = option.value_type || 'bool';
    const cppName = option.name.replace(/-/, '_');
    let valueType;
    if (typeof type === 'object') {
      if (type.enum_of != null) {
        const name = cppName;
        valueType = name;
        types.push({name, spec: {enum_of: type.enum_of.map(x => ({
          cliName: x,
          cppName: cppNameOf(x),
        })).sort()}});
      }
    } else {
      if (type === 'bool') {
        valueType = 'boolean';
      }
    }
    if (valueType == null) {
      throw Error(`invalid type for option ${options.name}`);
    }
    return {
      cppName,
      name: option.name,
      default: cppNameOf(option.default),
      valueType,
    };
  }).sort((a, b) => a.cppName > b.cppName ? 1 : -1);
  return {
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
  stream.write(`#include "${hppPath}"\n\n`);
  genNamespaceOpen(spec.namespace, stream);
  for (const type of spec.types) {
    if (type.spec.enum_of) {
      stream.write(`${type.name} parse_${type.name}(std::string str) {\n`);
      for (const variant of type.spec.enum_of) {
        stream.write(`  if (str == "${variant.cliName}") {\n`);
        stream.write(`    return ${type.name}::${variant.cppName};\n`);
        stream.write(`  }\n`);
      }
      stream.write(`  throw invalid_${type.name}_error(str);\n`);
      stream.write(`};\n\n`);
      continue;
    }
    throw new Error('unknown type');
  }
  stream.write(`options parse_options(const char* const argv[]) {
  options result;
  bool reading_options = true;
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
    if (option.valueType === 'boolean') {
      stream.write(`      result.${option.cppName} = true;\n`);
    } else {
      stream.write(`      ++argv;
      if (*argv == nullptr) {
        throw option_requires_argument_error("--${option.name}");
      }
`);
      stream.write(`      result.${option.cppName} = parse_${option.valueType}(*argv);\n`);
    }
    stream.write(`      continue;\n`);
    stream.write(`    }\n`);
  }
  stream.write(`    throw unexpected_argument_error(arg);
  }
  return result;
`);
  stream.write('}\n\n');
  genNamespaceClose(spec.namespace, stream);
}

function genCliHppParser(spec, stream) {
  stream.write(`#pragma once

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
    stream.write(`    ${option.cppName}(${option.valueType}::${option.default})`);
  }
  stream.write(' {}\n\n');
  stream.write(`  std::vector<std::string> rest_args;\n`);
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

`);
  stream.write('options parse_options(const char* const argv[]);\n\n');
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
