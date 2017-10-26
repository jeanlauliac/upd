/* @flow */

'use strict';

const chalk = require('chalk');
const util = require('util');
const path = require('path');

chalk.enabled = process.stderr.isTTY || false;

const ERROR_PREFIX = chalk.red('error:');
const MAIN_NAME = path.basename(process.mainModule.filename);

function error(format: string, ...placeholders: Array<mixed>) {
  const str = util.format(format, ...placeholders);
  console.error('%s: %s %s', MAIN_NAME, ERROR_PREFIX, str);
}

function fatalError(exitCode: number, format: string, ...placeholders: Array<mixed>) {
  process.exitCode = exitCode;
  error(format, ...placeholders);
}

module.exports = {error, fatalError};
