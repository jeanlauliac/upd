/* @flow */

'use strict';

import type {Location} from './types';

class UnexpectedCharError extends Error {
  char: string;
  location: Location;
  constructor(char: string, location: Location) {
    super(
      `${locationToString(location)}: unexpected ` +
        (char === '\n' ? 'newline' : `char \`${char}\``)
    );
    this.char = char;
    this.location = location;
  }
}

class UnexpectedEndError extends Error {
  location: Location;
  constructor(location: Location) {
    super(`${locationToString(location)}: unexpected end of file`);
    this.location = location;
  }
}

class NestedTestCaseError extends Error {
  location: Location;
  constructor(location: Location) {
    super(`${locationToString(location)}: nested @it() ` +
      `declarations are not allowed`);
    this.location = location;
  }
}

function locationToString(location: Location) {
  return `${location.line}:${location.column}`;
}

module.exports = {
  UnexpectedEndError,
  UnexpectedCharError,
  NestedTestCaseError,
  locationToString,
};
