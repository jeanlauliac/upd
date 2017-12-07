/* @flow */

'use strict';

const chalk = require('chalk');
const util = require('util');
const path = require('path');

chalk.enabled = process.stderr.isTTY || false;

const ERROR_PREFIX = chalk.red('error') + ':';
const FATAL_PREFIX = chalk.red('fatal') + ':';
const NAME = path.basename(process.mainModule.filename);

function error(format: string, ...placeholders: Array<mixed>) {
  const str = util.format(format, ...placeholders);
  console.error('%s: %s %s', NAME, ERROR_PREFIX, str);
}

function fatal(format: string, ...placeholders: Array<mixed>) {
  const str = util.format(format, ...placeholders);
  console.error('%s: %s %s', NAME, FATAL_PREFIX, str);
}

module.exports = {error, fatal, NAME, ERROR_PREFIX, chalk};
