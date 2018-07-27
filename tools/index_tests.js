#!/usr/bin/env node
/* @flow */

'use strict';

const cli = require('./lib/cli');
const fs = require('fs');
const path = require('path');
const hashString = require('./compile_test/hashString');
const reporting = require('./lib/reporting');
const writeNodeDepFile = require('./lib/writeNodeDepFile');

function writeContent(stream, sourcePaths, targetDirPath, testingHeaderPath) {
  const headerPath = path.relative(targetDirPath, testingHeaderPath);
  const entryPointNames = sourcePaths.map(sourceFilePath => {
    // FIXME: hash the file content, do not depend on any absolute paths, this
    // make the repository non-relocatable.
    // const absPath = path.resolve(sourceFilePath);
    const manifest = JSON.parse(fs.readFileSync(sourceFilePath, 'utf8'));
    return {
      name: `test_${manifest.id}`,
      filePath: path.relative(
        targetDirPath,
        path.join(path.dirname(sourceFilePath), manifest.sourceFilePath),
      ),
    };
  });
  stream.write(`#include "${headerPath}"\n`);
  stream.write(`#include "${path.dirname(headerPath)}/../../src/path.h"\n`);
  stream.write(`\n`);
  entryPointNames.forEach(entry => {
    stream.write(`void ${entry.name}(const std::string&, int&);\n`);
  });
  stream.write(`\n`);
  stream.write(`int main(int, char *argv[]) {\n`);
  stream.write(`  int index = 0;\n`);
  stream.write(`  std::string bin_path = upd::dirname(argv[0]);\n`);
  stream.write(`  testing::write_header();\n`);
  entryPointNames.forEach(entry => {
    stream.write(`  ${entry.name}(upd::normalize_path(bin_path + "/${entry.filePath}"), index);\n`);
  });
  stream.write(`  testing::write_plan(index);\n`);
  stream.write('}\n');
}

cli(function () {
  if (process.argv.length < 5) {
    reporting.fatal('not enough arguments');
    return 1;
  }
  const targetPath = process.argv[2];
  const depfilePath = process.argv[3];
  const headerPath = process.argv[4];
  const sourcePaths = process.argv.slice(5);
  let targetStream, targetDirPath;
  if (targetPath === '-') {
    targetStream = process.stdout;
    targetDirPath = process.cwd();
  } else {
    targetStream = fs.createWriteStream(targetPath);
    targetDirPath = path.dirname(targetPath);
  }
  writeContent(targetStream, sourcePaths, targetDirPath, headerPath);
  if (targetPath !== '-') {
    targetStream.end();
  }
  writeNodeDepFile(depfilePath, targetPath);
  return 0;
});
