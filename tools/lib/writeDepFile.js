'use strict';

function writeDepFile(stream, rules) {
  for (const rule of rules) {
    let first = true;
    for (const target of rule.targets) {
      if (!first) stream.write(' ');
      writeFilePath(stream, target);
      first = false;
    }
    stream.write(':');
    for (const dep of rule.dependencies) {
      stream.write(' ');
      writeFilePath(stream, dep);
    }
    stream.write('\n');
  }
  stream.end();
}

function writeFilePath(stream, filePath) {
  stream.write(filePath.replace(/([ :])/g, '\\\\$1'));
}

module.exports = writeDepFile;
