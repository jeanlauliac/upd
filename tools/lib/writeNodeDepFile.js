/* @flow */

'use strict';

const writeDepFile = require('./writeDepFile');
const fs = require('fs');

/**
 * Writes a depfile that describe all the Node.js modules currently loaded as
 * dependencies of the specified target file. For example if the current script
 * is "foo.js" and requires "bar.js" itself, it could write something like:
 *
 *     target.txt: foo.js bar.js
 *
 */
function writeNodeDepFile(depFilePath: string, targetFilePath: string) {
  writeDepFile(fs.createWriteStream(depFilePath), [{
    targets: [targetFilePath],
    dependencies: Object.values(require.cache)
      .filter(module => !/\/node_modules\//.test((module: $FlowFixMe).filename))
      .map((module: $FlowFixMe) => module.filename),
  }])
}

module.exports = writeNodeDepFile;
