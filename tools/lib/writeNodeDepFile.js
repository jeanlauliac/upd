/* @flow */

'use strict';

const writeDepFile = require('./writeDepFile');
const fs = require('fs');

/**
 * Writes a depfile that describe all the Node.js modules currently loaded as
 * dependencies of the specified target file.
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
