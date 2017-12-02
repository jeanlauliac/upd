/* @flow */

'use strict';

const {UnexpectedCharError} = require('./errors');
const CharIterator = require('./CharIterator');
const NonNullCharIterator = require('./NonNullCharIterator');
const path = require('path');

import type {Write, Location, LocatedChar, ReadLocatedChar} from './types';

type Options = {|
  +readLocatedChar: ReadLocatedChar,
  +write: Write,
  +targetDirPath: string,
  +sourceDirPath: string,
|};

class PreprocessorCharReader {
  _ops: Options;
  _expectSharp: boolean;

  constructor(options: Options) {
    this._ops = options;
    this._expectSharp = true;
  }

  next(): LocatedChar {
    const nIter = new CharIterator(this._ops.readLocatedChar);
    while (this._expectSharp && nIter.char === '#') {
      const iter = new NonNullCharIterator(nIter);
      this._ops.write('#');
      iter.forward();
      this._readWs(iter);
      let prType = '';
      while (/[a-z]/.test(iter.char)) {
        prType += iter.char;
        iter.forward();
      }
      if (prType !== 'include') {
        while (iter.char !== '\n') {
          this._ops.write(iter.char);
          iter.forward();
        }
        continue;
      }
      this._ops.write('include ');
      this._readWs(iter);
      const {sc, incPath} = this._readInclude(iter);
      this._readWs(iter);
      if (iter.char !== '\n')
        throw new UnexpectedCharError(iter.char, iter.location);
      const newIncPath = sc === '<' ? incPath : this._translateInclude(incPath);
      this._ops.write(`"${newIncPath}"\n`);
      nIter.forward();
    }
    this._expectSharp = (nIter.char === '\n');
    return {char: nIter.char, location: nIter.location};
  }

  _readWs(iter: NonNullCharIterator): void {
    while (/[ \t]/.test(iter.char)) iter.forward();
  }

  _readInclude(iter: NonNullCharIterator): {sc: string, incPath: string} {
    const sc = iter.char;
    if (!/["<]/.test(sc))
      throw new UnexpectedCharError(sc, iter.location);
    let incPath = '';
    iter.forward();
    while (iter.char !== '\n' &&
           sc === '"' ? (iter.char !== '"') : (iter.char !== '>')) {
      incPath += iter.char;
      iter.forward();
    }
    if (iter.char === '\n')
      throw new UnexpectedCharError(iter.char, iter.location);
    iter.forward();
    return {sc, incPath};
  }

  _translateInclude(incPath: string): string {
    const fullPath = path.resolve(this._ops.sourceDirPath, incPath);
    return path.relative(this._ops.targetDirPath, fullPath);
  }
}

module.exports = PreprocessorCharReader;
