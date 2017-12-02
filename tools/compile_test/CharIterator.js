/* @flow */

'use strict';

import type {Location, ReadLocatedChar} from './types';

class CharIterator {
  _read: ReadLocatedChar;
  char: ?string;
  location: Location

  constructor(readLocatedChar: ReadLocatedChar) {
    this._read = readLocatedChar;
    this.forward();
  }

  forward() {
    const lc = this._read();
    this.char = lc.char;
    this.location = lc.location;
  };
}

module.exports = CharIterator;
