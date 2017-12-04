/* @flow */

'use strict';

const crypto = require('crypto');
const cli = require('./lib/cli');
const fs = require('fs');
const path = require('path');
const reporting = require('./lib/reporting');
const writeNodeDepFile = require('./lib/writeNodeDepFile');

function main(argv: Array<string>) {
  if (argv.length !== 5) {
    reporting.fatal('wrong number of arguments');
    console.error(
      `usage: ${reporting.NAME} <target> <depfile> <source>`,
    );
    return 1;
  }
  const targetFilePath = process.argv[2];
  const depfilePath = process.argv[3];
  const sourceFilePath = process.argv[4];
  const relativePath = path.relative(path.dirname(targetFilePath),
                                     sourceFilePath);
  fs.writeFileSync(targetFilePath, JSON.stringify({
    sourceFilePath: relativePath,
    id: crypto.createHash('sha256').update(relativePath)
              .digest('hex').substr(0, 10),
  }));
  writeNodeDepFile(depfilePath, targetFilePath);
  return 0;
}

cli(main);
