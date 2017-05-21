#!/usr/bin/env node

'use strict';

const path = require('path');
const fs = require('fs');

function writeDepFile(depFilePath, destFilePath, depFilePaths) {
  const fileDepList = depFilePaths
    .map(filePath => filePath.replace(/ /g, '\\\\ '))
    .join('\\\n  ');
  const os = fs.createWriteStream(depFilePath);
  os.write(`${destFilePath}: ${fileDepList}\n`);
  os.end();
}

(function main() {
  const sourceFilePaths = process.argv.slice(4);
  const depFilePaths = [];
  const sources = sourceFilePaths.map(filePath => {
    const content = fs.readFileSync(filePath, 'utf8');
    const lines = content.split('\n');
    const result = [];
    for (let i = 0; i < lines.length; ++i) {
      const line = lines[i];
      const inc = "#include ";
      if (line.substring(0, inc.length) === inc) {
        const incName = line.substring(inc.length);
        const incPath = path.join(path.dirname(filePath), incName);
        const subcontent = fs.readFileSync(incPath, 'utf8');
        result.push(subcontent);
        depFilePaths.push(incPath);
        continue;
      }
      result.push(line);
      if (i < lines.length - 1) result.push('\n');
    }
    return '# ' + filePath + '\n' + result.join('');
  });
  const result = [
    '# GENERATED FILE\n',
  ].concat(sources).join('');
  const destFilePath = process.argv[2];
  const os = fs.createWriteStream(destFilePath);
  os.write(result);
  os.end();
  writeDepFile(process.argv[3], destFilePath, depFilePaths);
})();
