#!/usr/bin/env node

'use strict';

const cli = require('./lib/cli');
const fs = require('fs');
const path = require('path');
const reporting = require('./lib/reporting');

cli(function () {
  if (process.argv.length < 5) {
    reporting.fatalError(1, 'not enough arguments');
    return;
  }
  const targetPath = process.argv[2];
  const depfilePath = process.argv[3];
  const sourcePath = process.argv[4];
  const content = fs.readFileSync(sourcePath, 'utf8');
  const json = JSON.parse(content);
  const targetStream = fs.createWriteStream(targetPath);
  targetStream.write('namespace upd {\nnamespace package{\n\n');
  targetStream.write(`const char* NAME = ${JSON.stringify(json.name)};\n`);
  targetStream.write(`const char* VERSION = ${JSON.stringify(json.version)};\n`);
  targetStream.write(`const char* DESCRIPTION = ${JSON.stringify(json.description)};\n`);
  targetStream.write("}\n}\n");
  targetStream.end();
  const depfile = fs.createWriteStream(depfilePath);
  const modulePaths = Object.values(require.cache)
    .filter(module => !/\/node_modules\//.test(module.filename))
    .map(module => module.filename)
    .join(' \\\n  ');
  depfile.write(`${targetPath}: ${modulePaths}\n`);
  depfile.end();
});
