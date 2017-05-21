#!/usr/bin/env node

'use strict';

const child_process = require('child_process');
const fs = require('fs');
const path = require('path');
const rimraf = require('rimraf');

const ROOT_NAME = '.test-root';
const ROOT_PATH = path.resolve(__dirname, ROOT_NAME);
const UPDFILE = path.join(ROOT_PATH, 'updfile.json');

function expectToMatchSnapshot(name, filePath) {
  const result = fs.readFileSync(filePath, 'utf8');
  const snapshot = fs.readFileSync(path.join(__dirname, 'snapshots', name), 'utf8');
  if (snapshot !== result) {
    throw Error(`snapshot \`${name}' does not match file ${filePath}`);
  }
}

function runUpd(args) {
  const result = child_process.spawnSync(
    path.resolve(__dirname, '../dist/upd'),
    args,
    {cwd: ROOT_PATH, stdio: ['pipe', 'pipe', 'inherit']}
  );
  if (result.signal != null) {
    throw Error(`upd exited with signal ${result.signal}`);
  }
  if (result.status != 0) {
    throw Error(`upd exited with code ${result.status}`);
  }
  return result.stdout;
}

function runTestSuite() {
  rimraf.sync(ROOT_PATH);
  fs.mkdirSync(ROOT_PATH);
  fs.writeFileSync(UPDFILE, JSON.stringify({}));
  const reportedRootCout = runUpd(['--root']).toString('utf8');
  const reportedRoot = reportedRootCout.substr(0, reportedRootCout.length - 1);
  if (reportedRoot !== ROOT_PATH) {
    throw new Error('invalid root: `' + reportedRoot + '\'');
  }
  runUpd(['--all']);
  fs.writeFileSync(UPDFILE, JSON.stringify({
    "command_line_templates": [
      {
        "binary_path": "../mock_update.js",
        "arguments": [
          {
            "variables": ["output_file", "dependency_file", "input_files"]
          }
        ]
      }
    ],
    "source_patterns": ["src/(**/*).in"],
    "rules": [
      {
        "command_line_ix": 0,
        "inputs": [{"source_ix": 0}],
        "output": "dist/result.out"
      }
    ]
  }, null, 2));
  const srcDir = path.join(ROOT_PATH, 'src');
  fs.mkdirSync(srcDir);
  fs.writeFileSync(path.join(srcDir, 'foo.in'), 'This is foo.\n');
  fs.writeFileSync(path.join(srcDir, 'bar.in'), 'This is bar.\n');
  fs.mkdirSync(path.join(srcDir, 'sub'));
  fs.writeFileSync(path.join(srcDir, 'sub', 'glo.in'), 'This is glo.\n');
  runUpd(['dist/result.out']);
  expectToMatchSnapshot('first_result', path.join(ROOT_PATH, 'dist/result.out'));
  fs.writeFileSync(path.join(srcDir, 'foo.h'), 'Foo header.\n');
  fs.writeFileSync(path.join(srcDir, 'foo.in'), '#include foo.h\nThis is foo, second.\n');
  runUpd(['dist/result.out']);
  expectToMatchSnapshot('include_header_result', path.join(ROOT_PATH, 'dist/result.out'));
  fs.writeFileSync(path.join(srcDir, 'foo.h'), 'Foo header, modified.\n');
  runUpd(['dist/result.out']);
  expectToMatchSnapshot('header_modified_result', path.join(ROOT_PATH, 'dist/result.out'));
}

(function main() {
  runTestSuite();
  rimraf.sync(ROOT_PATH);
})();
