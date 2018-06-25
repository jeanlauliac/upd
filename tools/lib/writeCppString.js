/* @flow */

'use strict';

function writeCppString(write: (string) => void, str: string) {
  write('"');
  for (let i = 0; i < str.length; ++i) {
    const c = str.charAt(i);
    if (c === '"') write('\\"');
    else if (c === '\n') write('\\n');
    else if (c === '\\') write('\\\\');
    else write(c);
  }
  write('"');
}

module.exports = writeCppString;
