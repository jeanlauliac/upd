/* @flow */

'use strict';

const reporting = require('./reporting');

/**
 * Basic wrapper for command-line JavaScript tools.
 */
function cli(handler: () => number) {
  // $FlowFixMe: process should exist
  if (process.getuid() === 0) {
    reporting.fatal('cannot run as root');
    return 1;
  }
  if (parseInt(process.version.substr(1).split('.')[0]) !== 8) {
    reporting.fatal('requires node.js v8');
    return 1;
  }
  process.exitCode = handler();
}

module.exports = cli;
