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
  const reader = new StringCharReader(fs.readFileSync(sourcePath, 'utf8'));
  transform({
    readChar: reader.next.bind(reader),
    sourceFilePath: sourcePath,
    sourceDirPath: path.dirname(sourcePath),
    targetFilePath,
    headerFilePath,
    targetDirPath,
    write: data => void targetStream.write(data)
  });
  targetStream.end();
  writeNodeDepFile(depfilePath, targetFilePath);
  return 0;
}

class StringCharReader {
  _str: string;
  _ix: number;

  constructor(str: string) { this._str = str; this._ix = 0;}
  next(): ?string { return this._str[this._ix++]; }
}

type TransformOptions = {|
  +readChar: ReadChar,
  +sourceDirPath: string,
  +targetDirPath: string,
  +targetFilePath: string,
  +sourceFilePath: string,
  +headerFilePath: string,
  +write: Write,
|};

function transform(options: TransformOptions): void {
  const {sourceDirPath, targetDirPath, sourceFilePath, readChar} = options;
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
  let tests = [];
  let testBraceStack = 0;
  const iter = new CharIterator(reader.next.bind(reader));
  while (iter.char != null) {
    if (iter.char === '{' && testBraceStack > 0) {
      ++testBraceStack;
    }
    if (iter.char === '}' && testBraceStack > 0) {
      --testBraceStack;
    }
    // FIXME: '@' could appear in middle of strings, and comments. To properly
    // handle it we need a lightweight lexing layer below.
    if (iter.char !== '@') {
      write(iter.char);
      iter.forward();
      continue;
    }
    let markerLoc = iter.location;
    let marker = '';
    iter.forward();
    while (iter.char != null && /[a-z]/.test(iter.char)) {
      marker += iter.char;
      iter.forward();
    }
    if (marker == 'it') {
      if (testBraceStack > 0)
        throw new NestedTestCaseError(markerLoc);
      readWhitespace(iter);
      const title = readString(iter);
      readWhitespace(iter);
      if (iter.char !== '{') throw unexpectedOf(iter);
      iter.forward();
      const hash = hashString(title);
      const funcName = `test_case_${hash}`;
      write(`static void ${funcName}() {`);
      tests.push({hash, funcName, title});
      testBraceStack = 1;
      continue;
    }
    if (marker == 'expect') {
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
      write('testing::expect(\n');
      write(`#line ${expectExprLoc.line}\n`);
      write(' '.repeat(expectExprLoc.column - 1));
      write(expectExpr + ', ');
      writeString(write, expectExpr);
      write(`)`)
      continue;
    }
    write('@' + marker);
  }
  // FIXME: hash the file content, do not depend on any absolute paths, this
  // make the repository non-relocatable.
  const absPath = path.resolve(sourceFilePath);
  const mainFuncName = `test_${hashString(absPath)}`;
  write(`#line ${writer.line() + 1} "${options.targetFilePath}"\n`);
  write(`using namespace testing;\n`);
  write(`void ${mainFuncName}(int& index) {\n`);
  for (const test of tests) {
    write(`  run_case(${test.funcName}, index, `);
    writeString(write, test.title);
    write(`);\n`);
  }
  write(`}\n`);
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
