/* @flow */

'use strict';

const {UnexpectedEndError} = require('./errors');

import type CharIterator from './CharIterator';
import type {Location} from './types';

class NonNullCharIterator {
  _iter: CharIterator;
  char: string;
  location: Location

  constructor(iter: CharIterator) {
    this._iter = iter;
    this._loadChar();
  }

  forward() {
    this._iter.forward();
    this._loadChar();
  };

  _loadChar() {
    if (this._iter.char == null) {
      throw new UnexpectedEndError(this._iter.location);
    }
    this.char = this._iter.char;
    this.location = this._iter.location;
  }
}

module.exports = NonNullCharIterator;
