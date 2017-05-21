'use strict';

const chalk = require('chalk');
const util = require('util');
const path = require('path');

const ERROR_PREFIX = chalk.red('error:');
const MAIN_NAME = path.basename(process.mainModule.filename);

function error() {
  const str = util.format.apply(util, arguments);
  console.error('%s: %s %s', MAIN_NAME, ERROR_PREFIX, str);
}

function fatalError() {
  const args = Array.from(arguments);
  process.exitCode = args.shift();
  error.apply(this, args);
}

module.exports = {error, fatalError};
