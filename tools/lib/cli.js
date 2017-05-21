'use strict';

const reporting = require('./reporting');

function cli(callback) {
  if (process.getuid() === 0) {
    reporting.fatalError(1, 'cannot run as root');
    return;
  }
  if (parseInt(process.version.substr(1).split('.')[0]) !== 7) {
    reporting.fatalError(1, 'requires node.js v7');
    return;
  }
  callback();
}

module.exports = cli;
