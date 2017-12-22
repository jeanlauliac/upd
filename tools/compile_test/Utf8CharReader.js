/* @flow */

'use strict';

type ReadByte = () => ?number;

/**
 * We read one proper character at a time that allows us to have the correct
 * column number. If we were iterating a JavaScript string instead, we would
 * not get proper column numbers as strings are stored with UTF16 encoding.
 */
class Utf8CharReader {
  _readByte: ReadByte;
  _buf: Buffer;

  constructor(readByte: ReadByte) {
    this._readByte = readByte;
    this._buf = new Buffer(4);
  }

  next(): ?string {
    const byte = this._readByte();
    if (byte == null) return null;
    if ((byte & 0b10000000) === 0) return String.fromCodePoint(byte);
    if ((byte & 0b01000000) === 0) {
      throw new Error('invalid byte sequence when decoding utf8');
    }
    this._buf[0] = byte;
    this._buf[1] = this._readNonNullByte();
    if ((byte & 0b00100000) === 0) return this._buf.toString('utf8', 0, 2);
    this._buf[2] = this._readNonNullByte();
    if ((byte & 0b00010000) === 0) return this._buf.toString('utf8', 0, 3);
    this._buf[3] = this._readNonNullByte();
    return this._buf.toString('utf8', 0, 4);
  }

  _readNonNullByte(): number {
    const byte = this._readByte();
    if (byte == null)
      throw new Error('unexpected end-of-file when decoding utf8');
    return byte;
  }
}

module.exports = Utf8CharReader;
