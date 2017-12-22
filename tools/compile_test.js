#!/usr/bin/env node
/* @flow */

'use strict';

const CharIterator = require('./compile_test/CharIterator');
const {UnexpectedCharError, UnexpectedEndError,
       NestedTestCaseError, locationToString} = require('./compile_test/errors');
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
  const fdReader = new FdChunkReader(fd);
  const byteReader = new ChunkToByteReader(fdReader.read.bind(fdReader));
  const btcReader = new UTF8Reader(byteReader.next.bind(byteReader));
  const readChar = btcReader.next.bind(btcReader);
  const locrd = new LocationTrackingCharReader(readChar);
  const ctcr = new ContextTrackingCharReader(locrd);
  const rpe = reportError.bind(null, sourcePath, ctcr);
  try {
    transform({
      readChar: ctcr.next.bind(ctcr),
      sourceFilePath: sourcePath,
      sourceDirPath: path.dirname(sourcePath),
      targetFilePath,
      headerFilePath,
      targetDirPath,
      write: data => void targetStream.write(data)
    });
  } catch (error) {
    if (error instanceof UnexpectedEndError) {
      rpe(error.location, 'unexpected end of file');
    } else if (error instanceof UnexpectedCharError) {
      rpe(error.location, `unexpected character \`${error.char}\``);
    } else {
      throw error;
    }
    return 2;
  } finally {
    fs.closeSync(fd);
    targetStream.end();
  }
  writeNodeDepFile(depfilePath, targetFilePath);
  return 0;
}

function reportError(
  sourcePath: string,
  ctcr: ContextTrackingCharReader,
  location: Location,
  message: string,
) {
  process.stderr.write(`${sourcePath}:${locationToString(location)}: ` +
    `${reporting.ERROR_PREFIX} ${reporting.chalk.bold(message)}\n`);
  const ls = ctcr.getLineString();
  if (ls === '') return;
  const lstr = location.line.toString() + ':  ';
  process.stderr.write(lstr + ls);
  process.stderr.write(' '.repeat(location.column + lstr.length - 1) + '^\n');
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

class UTF8Reader {
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
      throw new Error('invalid byte sequence when reading utf8');
    }
    this._buf[0] = byte;
    this._buf[1] = this._readByte();
    if ((byte & 0b00100000) === 0) return this._buf.toString('utf8', 0, 2);
    this._buf[2] = this._readByte();
    if ((byte & 0b00010000) === 0) return this._buf.toString('utf8', 0, 3);
    this._buf[3] = this._readByte();
    return this._buf.toString('utf8', 0, 4);
  }
}

function utf8e(byte) {
  return byte & 0b00111111;
}

type TransformOptions = {|
  +readChar: ReadLocatedChar,
  +sourceDirPath: string,
  +targetDirPath: string,
  +targetFilePath: string,
  +sourceFilePath: string,
  +headerFilePath: string,
  +write: Write,
|};

function transform(options: TransformOptions): void {
  const {sourceDirPath, targetDirPath, sourceFilePath} = options;
  const writer = new LineTrackingWriter(options.write);
  const write = writer.write.bind(writer);
  const headerFilePath = path.relative(targetDirPath, options.headerFilePath);
  write(`#include "${headerFilePath}"\n`);
  write(`#line 1 "${sourceFilePath}"\n`);
  const iter = new CharIterator(options.readChar);
  const tr = new Transformer(iter, write, {sourceDirPath, targetDirPath});
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

type Options = {|
  +targetDirPath: string,
  +sourceDirPath: string,
|};

class Transformer {
  _iter: CharIterator;
  _write: Write;
  _opts: Options;
  _testBraceStack: number;
  _beginLine: boolean;
  tests: Array<{hash: string, funcName: string, title: string}>;

  constructor(iter, write, options) {
    this._iter = iter;
    this._write = write;
    this._testBraceStack = 0;
    this._opts = options;
    this.tests = [];
    this._beginLine = true;
  }

  run() {
    while (this._iter.char != null) {
      this._processChar(this._iter.char);
    }
  }

  _processChar(c: string) {
    const iter = this._iter;
    if (c === '#' && this._beginLine) {
      this._processInclude();
      return;
    }
    this._beginLine = (c === '\n');
    if (c === '{' && this._testBraceStack > 0) {
      ++this._testBraceStack;
    }
    if (c === '}' && this._testBraceStack > 0) {
      --this._testBraceStack;
    }
    if (c === '"') {
      this._skipString();
      return;
    }
    if (c === '/') {
      this._write('/');
      iter.forward();
      if (iter.char === '*') {
        this._write('*');
        this._skipComment();
      } else if (iter.char === '/') {
        this._write('/');
        this._skipLineComment();
      }
      return;
    }
    if (c !== '@') {
      this._write(c);
      iter.forward();
      return;
    }
    this._processMarker();
  }

  _skipString() {
    const iter = this._iter;
    this._write('"');
    iter.forward();
    while (iter.char != null && iter.char !== '"') {
      if (iter.char === '\\') {
        this._write(iter.char);
        iter.forward();
      }
      if (iter.char == null)
        throw new UnexpectedEndError(iter.location);
      this._write(iter.char);
      iter.forward();
    }
    if (iter.char == null)
      throw new UnexpectedEndError(iter.location);
    this._write(iter.char);
    iter.forward();
  }

  _skipComment() {
    const iter = this._iter;
    iter.forward();
    let stage = 0;
    while (iter.char != null && stage < 2) {
      switch (stage) {
        case 0:
          if (iter.char === '*') stage = 1;
          break;
        case 1:
          if (iter.char === '/') stage = 2;
          else stage = 0;
          break;
      }
      this._write(iter.char);
      iter.forward();
    }
    if (iter.char == null)
      throw new UnexpectedEndError(iter.location);
  }

  _skipLineComment() {
    const iter = this._iter;
    iter.forward();
    while (iter.char != null && iter.char !== '\n') {
      this._write(iter.char);
      iter.forward();
    }
  }

  _processInclude() {
    const iter = this._iter;
    this._write('#');
    iter.forward();
    this._skipSpaces();
    let instruction = '';
    while (iter.char != null && /[a-z]/.test(iter.char)) {
      instruction += iter.char;
      this._write(iter.char);
      iter.forward();
    }
    if (instruction !== 'include') {
      this._skipUntilNewline();
      return;
    }
    this._skipSpaces();
    if (iter.char !== '"') {
      this._skipUntilNewline();
      return;
    }
    let filePath = '';
    iter.forward();
    while (iter.char != null && iter.char !== '\n' && iter.char !== '"') {
      filePath += iter.char;
      iter.forward();
    }
    if (iter.char !== '"') return;
    iter.forward();
    this._write(`"${this._translateInclude(filePath)}"`);
    this._skipUntilNewline();
  }

  _skipSpaces() {
    const iter = this._iter;
    while (iter.char != null && /[ \t]/.test(iter.char)) {
      this._write(iter.char);
      iter.forward();
    }
  }

  _skipUntilNewline() {
    const iter = this._iter;
    while (iter.char != null && iter.char !== '\n') {
      this._write(iter.char);
      iter.forward();
    }
  }

  _translateInclude(incPath: string): string {
    const fullPath = path.resolve(this._opts.sourceDirPath, incPath);
    return path.relative(this._opts.targetDirPath, fullPath);
  }

  _processMarker() {
    const iter = this._iter;
    let markerLoc = iter.location;
    let marker = '';
    iter.forward();
    while (iter.char != null && /[a-z]/.test(iter.char)) {
      marker += iter.char;
      iter.forward();
    }
    switch (marker) {
    case 'it': return this._processItMarker(markerLoc);
    case 'assert': return this._processAssertMarker(markerLoc);
    case 'expect': return this._processExpectMarker(markerLoc);
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
    const {expr, loc} = this._readParenExpression();
    this._write('testing::assert(\n');
    this._write(`#line ${loc.line}\n`);
    this._write(' '.repeat(loc.column - 1));
    this._write(expr + ', ');
    writeString(this._write, expr);
    this._write(`)`);
  }

  _processExpectMarker(markerLoc: Location) {
    const iter = this._iter;
    readWhitespace(iter);
    const actualValue = this._readParenExpression();
    readWhitespace(iter);
    if (iter.char !== '.') throw unexpectedOf(iter);
    iter.forward();
    readWhitespace(iter);
    let operator = '';
    while (iter.char != null && /[a-z_]/.test(iter.char)) {
      operator += iter.char;
      iter.forward();
    }
    if (operator != 'to_equal')
      throw new Error(`unknown @expect operator \`${operator}\``);
    const expectedValue = this._readParenExpression();
    this._write('testing::expect_equal(\n');
    this._write(`#line ${actualValue.loc.line}\n`);
    this._write(' '.repeat(actualValue.loc.column - 1));
    this._write(actualValue.expr + ',\n');
    this._write(`#line ${expectedValue.loc.line}\n`);
    this._write(' '.repeat(expectedValue.loc.column - 1));
    this._write(expectedValue.expr + ', ');
    writeString(this._write, actualValue.expr);
    this._write(', ');
    writeString(this._write, expectedValue.expr);
    this._write(`)`);
  }

  // FIXME: ignore parens inside comments or strings.
  _readParenExpression() {
    const iter = this._iter;
    if (iter.char !== '(') throw unexpectedOf(iter);
    iter.forward();
    const loc = iter.location;
    let parenStack = 0;
    let expr = '';
    while (iter.char != null && !(parenStack === 0 && iter.char === ')')) {
      expr += iter.char;
      if (iter.char === '(') ++parenStack;
      if (iter.char === ')') --parenStack;
      iter.forward();
    }
    if (iter.char == null) throw new UnexpectedEndError(iter.location);
    iter.forward();
    return {expr, loc};
  }
}

function readWhitespace(iter: CharIterator) {
  while (iter.char != null && /[ \t\n]/.test(iter.char)) iter.forward();
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

class ContextTrackingCharReader {
  _ltcr: LocationTrackingCharReader;
  _line: Array<LocatedChar>;
  _ix: 0;
  _next: LocatedChar;

  constructor(ltcr: LocationTrackingCharReader) {
    this._ltcr = ltcr;
    this._line = [];
    this._ix = 0;
    this._next = ltcr.next();
  }

  next(): LocatedChar {
    if (this._ix >= this._line.length) {
      this._readLine();
    }
    return this._line[this._ix++];
  }

  getLineString(): string {
    return this._line.map(x => x.char || '').join('');
  }

  _readLine() {
    this._ix = 0;
    this._line = [this._next];
    if (this._next.char == null) {
      return;
    }
    let lc = this._ltcr.next();
    while (lc.char != null && lc.location.line === this._next.location.line) {
      this._line.push(lc);
      lc = this._ltcr.next();
    }
    this._next = lc;
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
