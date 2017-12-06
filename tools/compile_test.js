#!/usr/bin/env node
/* @flow */

'use strict';

const CharIterator = require('./compile_test/CharIterator');
const PreprocessorCharReader = require('./compile_test/PreprocessorCharReader');
const {UnexpectedCharError, UnexpectedEndError,
       NestedTestCaseError} = require('./compile_test/errors');
const crypto = require('crypto');
const fs = require('fs');
const path = require('path');
const hashString = require('./compile_test/hashString');
const reporting = require('./lib/reporting');
const writeNodeDepFile = require('./lib/writeNodeDepFile');

import type {ReadChar, Write, Location, LocatedChar, ReadLocatedChar} from './compile_test/types';

function main() {
  if (process.argv.length < 6) {
    reporting.fatal('wrong number of arguments');
    console.error(
      'usage: ' + reporting.NAME +
        ' <source> <target> <depfile> <test_header>',
    );
    return 1;
  }
  const sourcePath = process.argv[2];
  const targetFilePath = process.argv[3];
  const depfilePath = process.argv[4];
  const headerFilePath = process.argv[5];
  const targetStream = fs.createWriteStream(targetFilePath);
  const targetDirPath = path.dirname(targetFilePath);
  const fd = fs.openSync(sourcePath, 'r');
  try {
    const fdReader = new FdChunkReader(fd);
    transform({
      readChunk: fdReader.read.bind(fdReader),
      sourceFilePath: sourcePath,
      sourceDirPath: path.dirname(sourcePath),
      targetFilePath,
      headerFilePath,
      targetDirPath,
      write: data => void targetStream.write(data)
    });
  } finally {
    fs.closeSync(fd);
  }
  targetStream.end();
  writeNodeDepFile(depfilePath, targetFilePath);
  return 0;
}

class FdChunkReader {
  _fd: number;

  constructor(fd: number) {
    this._fd = fd;
  }

  read(buffer: Buffer): number {
    return fs.readSync(this._fd, buffer, 0, buffer.length, (null: $FlowFixMe));
  }
}

type ReadChunk = (buffer: Buffer) => number;

class ChunkToByteReader {
  _readChunk: ReadChunk;
  _buf: Buffer;
  _count: number;
  _ix: number;

  constructor(readChunk: ReadChunk) {
    this._readChunk = readChunk;
    this._buf = new Buffer(1 << 12);
    this._count = 0
    this._ix = 0;
  }

  next(): ?number {
    if (this._ix >= this._count) {
      this._count = this._readChunk(this._buf);
      if (this._count === 0) {
        return null;
      }
      this._ix = 0;
    }
    return this._buf[this._ix++];
  }
}

type ReadByte = () => ?number;

class ByteToCharReader {
  _readByte: ReadByte;

  constructor(readByte: ReadByte) {
    this._readByte = readByte;
  }

  next(): ?string {
    const byte = this._readByte();
    if (byte == null) return null;
    return String.fromCharCode(byte);
  }
}

type TransformOptions = {|
  +readChunk: ReadChunk,
  +sourceDirPath: string,
  +targetDirPath: string,
  +targetFilePath: string,
  +sourceFilePath: string,
  +headerFilePath: string,
  +write: Write,
|};

function transform(options: TransformOptions): void {
  const {readChunk} = options;
  const byteReader = new ChunkToByteReader(readChunk);
  const btcReader = new ByteToCharReader(byteReader.next.bind(byteReader));
  const readChar = btcReader.next.bind(btcReader);
  const {sourceDirPath, targetDirPath, sourceFilePath} = options;
  const writer = new LineTrackingWriter(options.write);
  const write = writer.write.bind(writer);
  const headerFilePath = path.relative(targetDirPath, options.headerFilePath);
  write(`#include "${headerFilePath}"\n`);
  write(`#line 1 "${sourceFilePath}"\n`);
  let locrd = new LocationTrackingCharReader(readChar);
  let reader = new PreprocessorCharReader({
    readLocatedChar: locrd.next.bind(locrd),
    write,
    sourceDirPath,
    targetDirPath,
  });
  const iter = new CharIterator(reader.next.bind(reader));
  const tr = new Transformer(iter, write);
  tr.run();
  const relPath = path.relative(targetDirPath, options.sourceFilePath);
  const testId = crypto.createHash('sha256').update(relPath)
                       .digest('hex').substr(0, 10);
  const mainFuncName = `test_${testId}`;
  write(`#line ${writer.line() + 1} "${options.targetFilePath}"\n`);
  write(`using namespace testing;\n`);
  write(`void ${mainFuncName}(int& index) {\n`);
  for (const test of tr.tests) {
    write(`  run_case(${test.funcName}, index, `);
    writeString(write, test.title);
    write(`);\n`);
  }
  write(`}\n`);
}

class Transformer {
  _iter: CharIterator;
  _write: Write;
  _testBraceStack: number;
  tests: Array<{hash: string, funcName: string, title: string}>;

  constructor(iter, write) {
    this._iter = iter;
    this._write = write;
    this._testBraceStack = 0;
    this.tests = [];
  }

  run() {
    while (this._iter.char != null) {
      this._processChar(this._iter.char);
    }
  }

  _processChar(c: string) {
    const iter = this._iter;
    if (c === '{' && this._testBraceStack > 0) {
      ++this._testBraceStack;
    }
    if (c === '}' && this._testBraceStack > 0) {
      --this._testBraceStack;
    }
    // FIXME: '@' could appear in middle of strings, and comments. To properly
    // handle it we need a lightweight lexing layer below.
    if (c !== '@') {
      this._write(c);
      iter.forward();
      return;
    }
    let markerLoc = iter.location;
    let marker = '';
    iter.forward();
    while (iter.char != null && /[a-z]/.test(iter.char)) {
      marker += iter.char;
      iter.forward();
    }
    if (marker == 'it') {
      this._processItMarker(markerLoc);
      return;
    }
    if (marker == 'assert') {
      this._processAssertMarker(markerLoc);
      return;
    }
    this._write('@' + marker);
  }

  _processItMarker(markerLoc: Location) {
    const iter = this._iter;
    if (this._testBraceStack > 0)
      throw new NestedTestCaseError(markerLoc);
    readWhitespace(iter);
    const title = readString(iter);
    readWhitespace(iter);
    if (iter.char !== '{') throw unexpectedOf(iter);
    iter.forward();
    const hash = hashString(title);
    const funcName = `test_case_${hash}`;
    this._write(`static void ${funcName}() {`);
    this.tests.push({hash, funcName, title});
    this._testBraceStack = 1;
  }

  _processAssertMarker(markerLoc: Location) {
    const iter = this._iter;
    readWhitespace(iter);
    if (iter.char !== '(') throw unexpectedOf(iter);
    iter.forward();
    const expectExprLoc = iter.location;
    let parenStack = 0;
    let expectExpr = '';
    while (iter.char != null && !(parenStack === 0 && iter.char === ')')) {
      expectExpr += iter.char;
      if (iter.char === '(') ++parenStack;
      if (iter.char === ')') --parenStack;
      iter.forward();
    }
    if (iter.char == null)
      throw new UnexpectedEndError(iter.location);
    iter.forward();
    this._write('testing::assert(\n');
    this._write(`#line ${expectExprLoc.line}\n`);
    this._write(' '.repeat(expectExprLoc.column - 1));
    this._write(expectExpr + ', ');
    writeString(this._write, expectExpr);
    this._write(`)`);
  }
}

function readWhitespace(iter: CharIterator) {
  while (iter.char != null && /[ \n]/.test(iter.char)) iter.forward();
}

function readString(iter: CharIterator) {
  if (iter.char !== '"') throw unexpectedOf(iter);
  iter.forward();
  let str = '';
  while (iter.char !== '"') {
    if (iter.char === '\\') {
      iter.forward();
    }
    if (iter.char == null)
      throw new UnexpectedEndError(iter.location)
    str += iter.char;
    iter.forward();
  }
  iter.forward();
  return str;
}

function writeString(write: Write, str: string) {
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

function unexpectedOf(iter: CharIterator) {
  if (iter.char != null)
    throw new UnexpectedCharError(iter.char, iter.location);
  throw new UnexpectedEndError(iter.location);
}

class LocationTrackingCharReader {
  _readChar: ReadChar;
  _nextLoc: Location;

  constructor(readChar: ReadChar) {
    this._readChar = readChar;
    this._nextLoc = {column: 1, line: 1, index: 0};
  }

  next(): LocatedChar {
    const c = this._readChar();
    const {column, line, index} = this._nextLoc;
    if (c === '\n') {
      this._nextLoc = {column: 1, line: line + 1, index: index + 1};
    } else {
      this._nextLoc = {column: column + 1, line, index: index + 1};
    }
    return {char: c, location: {column, line, index}};
  }
}

class LineTrackingWriter {
  _write: Write;
  _line: number;

  constructor(write: Write) {
    this._write = write;
    this._line = 1;
  }

  write(content: string): void {
    this._line += (content.match(/\n/g) || []).length;
    this._write(content);
  }

  line() { return this._line; }
}

require('./lib/cli')(main);
