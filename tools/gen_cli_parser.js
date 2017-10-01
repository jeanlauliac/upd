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
        const name = cppName + '_t'
        valueType = name;
        types.push({name, spec: {enum_of: type.enum_of.map(cppNameOf).sort()}});
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
      default: option.default,
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
  stream.write(`namespace ${spec.namespace} {\n\n`);
  stream.write(`options parse_options(const char* const argv[]) {
  options result;
  bool reading_options = true;
  for (++argv; *argv != nullptr; ++argv) {
    const auto arg = std::string(*argv);
    if (!reading_options || arg.size() == 0 || arg[0] != '-') {
      result.rest_args.push_back(arg);
    }
    if (arg.size() == 1 || arg[1] != '-') {
      throw unexpected_argument_error(arg);
    }
`);
  for (const option of spec.options) {
    stream.write(`    if (arg.compare(2, arg.size(), '${option.name}') == 0) {\n\n`);
    stream.write(`    }\n`);
  }
  stream.write(`  }
  return result;
`);
  stream.write('}\n\n');
  stream.write('}\n');
}

function genCliHppParser(spec, stream) {
  stream.write('#pragma once\n\n');
  stream.write(`namespace ${spec.namespace} {\n\n`);
  for (const type of spec.types) {
    if (type.spec.enum_of) {
      stream.write(`enum class ${type.name} {\n`);
      for (const variant of type.spec.enum_of) {
        stream.write(`  ${variant},\n`);
      }
      stream.write(`}\n\n`);
    }
  }
  stream.write('struct options {\n');
  for (const option of spec.options) {
    stream.write(`  std::vector<std::string> rest_args;\n`);
    stream.write(`  ${option.valueType} ${option.cppName};\n`);
  }
  stream.write('}\n\n');
  stream.write('options parse_options(const char* const argv[]);\n\n')
  stream.write('}\n');
}
