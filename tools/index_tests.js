#!/usr/bin/env node

'use strict';

const cli = require('./lib/cli');
const fs = require('fs');
const path = require('path');
const reporting = require('./lib/reporting');

function writeContent(stream, sourcePaths, targetDirPath) {
  const headerPath = path.relative(targetDirPath, path.resolve(__dirname, 'lib/testing.h'));
  const entryPointNames = sourcePaths.map(sourcePath => {
    return 'test_' + sourcePath.replace(/\//g, 'zS').replace(/\./g, 'zD').replace(/\-/g, 'zN');
  });
  stream.write(`#include "${headerPath}"\n`);
  stream.write(`\n`);
  entryPointNames.forEach(name => {
    stream.write(`void ${name}(int&);\n`);
  });
  stream.write(`\n`);
  stream.write(`int main() {\n`);
  stream.write(`  int index = 0;\n`);
  stream.write(`  testing::write_header();\n`);
  entryPointNames.forEach(name => {
    stream.write(`  ${name}(index);\n`);
  });
  stream.write(`  testing::write_plan(index);\n`);
  stream.write('}\n');
}

cli(function () {
  if (process.argv.length < 4) {
    reporting.fatalError(1, 'not enough arguments');
    return;
  }
  const targetPath = process.argv[2];
  const depfilePath = process.argv[3];
  const sourcePaths = process.argv.slice(4);
  let targetStream, targetDirPath;
  if (targetPath === '-') {
    targetStream = process.stdout;
    targetDirPath = process.cwd();
  } else {
    targetStream = fs.createWriteStream(targetPath);
    targetDirPath = path.dirname(targetPath);
  }
  writeContent(targetStream, sourcePaths, targetDirPath);
  if (targetPath !== '-') {
    targetStream.end();
  }
  const depfile = fs.createWriteStream(depfilePath);
  const modulePaths = Object.values(require.cache)
    .filter(module => !/\/node_modules\//.test(module.filename))
    .map(module => module.filename)
    .join(' \\\n  ');
  depfile.write(`${targetPath}: ${modulePaths}\n`);
  depfile.end();
});
