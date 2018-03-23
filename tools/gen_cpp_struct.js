/* @flow */

'use strict';

const cli = require('./lib/cli');
const fs = require('fs');
const path = require('path');
const reporting = require('./lib/reporting');
const writeNodeDepFile = require('./lib/writeNodeDepFile');
const json5 = require('json5');

cli(function () {
  if (process.argv.length < 5) {
    reporting.fatal('not enough arguments');
    return 1;
  }
  const targetPath = process.argv[2];
  const depfilePath = process.argv[3];
  const sourcePath = process.argv[4];
  const content = fs.readFileSync(sourcePath, 'utf8');
  const json = json5.parse(content);
  const targetStream = fs.createWriteStream(targetPath);
  const write = targetStream.write.bind(targetStream);
  write('#pragma once\n\n');
  const originDirPath = path.dirname(sourcePath);
  const targetDirPath = path.dirname(targetPath);
  json.includes.forEach(incPath => {
    if (incPath.includes('/')) {
      write(`#include "${path.relative(targetDirPath, path.join(originDirPath, incPath))}"\n`);
    } else {
      write(`#include <${incPath}>\n`);
    }
  });
  write('\n');
  json.namespace.forEach(name => write(`namespace ${name} {\n`));
  write('\n');
  const ns = json.namespace.join('::');
  (json.enums || []).forEach(spec => {
    write(`enum class ${spec.name} {\n`);
    spec.values.forEach(value => {
      write(`  ${value},\n`);
    });
    write(`};\n\n`);
    write(`inline std::string inspect(const ${spec.name}& value, const inspect_options &options) {
  switch (value) {\n`);
    spec.values.forEach(value => {
      const qualifiedName = `${spec.name}::${value}`;
      write(`    case ${qualifiedName}: return "${ns}::${qualifiedName}";\n`);
    });
    write(`  }\n`);
    write(`  throw std::runtime_error('invalid enum value');\n`)
    write(`}\n\n`);
  });
  (json.structs || []).forEach(spec => {
    write(`struct ${spec.name} {\n`);
    spec.fields.forEach(fspec => {
      write(`  ${fspec.type} ${fspec.name};\n`);
    });
    write(`};\n\n`);
    write(`inline bool operator==(const ${spec.name} &left, const ${spec.name} &right) {\n`);
    write(`  return `);
    let first = true;
    spec.fields.forEach(fspec => {
      if (!first) write(` &&\n    `);
      write(`left.${fspec.name} == right.${fspec.name}`);
      first = false;
    });
    write(`;\n}\n`);
    write(`
inline std::string inspect(const ${spec.name}& value, const inspect_options &options) {
  collection_inspector insp("${ns}::${spec.name}", options);
`);
    spec.fields.forEach(fspec => {
      write(`  insp.push_back("${fspec.name}", value.${fspec.name});\n`);
    });
    write(`  return insp.result();\n}\n\n`);
  });
  json.namespace.forEach(name => write(`} // namespace ${name}\n`));
  targetStream.end();
  writeNodeDepFile(depfilePath, targetPath);
  return 0;
});
