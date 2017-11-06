#!/usr/bin/env node
/* @flow */

'use strict';

const cli = require('./lib/cli');
const fs = require('fs');
const path = require('path');
const reporting = require('./lib/reporting');
const writeNodeDepFile = require('./lib/writeNodeDepFile');

function fatalError(filePath, content, i, str) {
  const lineNumber = (content.substr(0, i).match(/\n/g) || []).length;
  reporting.fatalError(2, '%s:%s: %s', filePath, lineNumber, str);
}

function readCppString(content, i, filePath) {
  if (content[i] !== '"') {
    fatalError(filePath, content, i, 'expected string');
    return null;
  }
  const j = content.indexOf('"', i + 1);
  if (j === -1) {
    fatalError(filePath, content, i, 'string lacks closing quote');
    return null;
  }
  return [content.substring(i + 1, j), j + 1];
}

type Location = {line: number};

function readCppBlock(content, i, filePath, loc: Location) {
  if (content[i] !== '{') {
    fatalError(filePath, content, i, 'expected block after @it title');
    return null;
  }
  i += 1;
  const blockStart = i;
  let braceCount = 1;
  while (braceCount > 0 && i < content.length) {
    if (content[i] === '{') {
      ++braceCount;
    } else if (content[i] === '}') {
      --braceCount;
    } else if (content[i] === '\n') {
      ++loc.line;
    }
    ++i;
  }
  if (braceCount > 0) {
    fatalError(filePath, content, i, 'block after @it title isn\'t closed');
    return null;
  }
  return [content.substring(blockStart, i - 1), i];
}

function readCppParenExpr(content, i, filePath) {
  if (content[i] !== '(') {
    fatalError(filePath, content, i, 'expected parenthese after @expect');
    return null;
  }
  i += 1;
  const start = i;
  let parenCount = 1;
  while (parenCount > 0 && i < content.length) {
    if (content[i] === '(') {
      ++parenCount;
    } else if (content[i] === ')') {
      --parenCount;
    } else if (content[i] === '"') {
      ++i;
      while (i < content.length && content[i] !== '"') {
        if (content[i] === '\\') ++i;
        ++i;
      }
    }
    ++i;
  }
  if (parenCount > 0) {
    fatalError(filePath, content, i, 'unclosed parenthese for @expect');
    return null;
  }
  return [content.substring(start, i - 1), i];
}

function writeTestFunction(stream, caseCount, caseName, blockContent, filePath, relFilePath: string, startLine: number) {
  const functionName = `test_case_${caseCount}`;
  stream.write(`static void ${functionName}() {\n`);
  stream.write(`#line ${startLine} "${path.resolve(filePath)}"\n`);
  let i = 0;
  while (i < blockContent.length) {
    let j = blockContent.indexOf('@expect', i);
    const unchangedContent = blockContent.substring(i, j >= 0 ? j : blockContent.length);
    stream.write(unchangedContent);
    if (j < 0) {
      i = blockContent.length;
      continue;
    }
    i = j + 7;
    let cppExpr = readCppParenExpr(blockContent, i, filePath);
    if (cppExpr == null) {
      continue;
    }
    let exprStr;
    [exprStr, i] = cppExpr;
    const strContent = exprStr.replace(/([\\"])/g, '\\$1').replace(/\n/g, '\\n');
    // Quick-and-dirty, of course we'd want to check parentheses and stuff.
    const dbEqualIx = exprStr.indexOf('==');
    if (dbEqualIx >= 0) {
      const targetExp = exprStr.substring(0, dbEqualIx);
      const expectationExp = exprStr.substring(dbEqualIx + 2);
      stream.write(`testing::expect_equal(${targetExp}, ${expectationExp}, "${strContent}")`);
    } else {
      stream.write(`testing::expect(${exprStr}, "${strContent}")`);
    }
  }
  stream.write('}');
  return functionName;
}

function writeHeader(stream, targetDirPath, testingHeaderPath) {
  const headerPath = path.relative(targetDirPath, testingHeaderPath);
  stream.write('#include <iostream>\n');
  stream.write(`#include "${headerPath}"\n`);
  stream.write(`\n`);
}

function writeMain(stream, testFunctions, filePath) {
  // TODO: look at Flow's stategy for escaping slashes and dots
  const entryPointName = 'test_' + filePath.replace(/\//g, 'zS').replace(/\./g, 'zD').replace(/\-/g, 'zN');
  stream.write(`void ${entryPointName}(int& index) {\n`);
  for (let i = 0; i < testFunctions.length; i++) {
    const fun = testFunctions[i];
    stream.write(`  testing::run_case(${fun.functionName}, index, "${filePath}: ${fun.name}");\n`)
  }
  stream.write('}\n');
}

const IT_MARKER = '@it ';

function startsWith(content, marker, contentIx) {
  let i = 0;
  while (
    contentIx < content.length &&
    i < marker.length &&
    content[contentIx] === marker[i]
  ) {
    ++i;
    ++contentIx;
  }
  return i === marker.length;
}

function transform(content, stream, filePath, targetDirPath, headerPath) {
  const relFilePath = path.relative(targetDirPath, filePath);
  let loc = {line: 1};
  let i = 0;
  let caseCount = 0;
  const testFunctions = [];
  writeHeader(stream, targetDirPath, headerPath);
  while (i < content.length) {
    let j = i;
    let startLine = loc.line;
    let found = startsWith(content, IT_MARKER, j);
    while (j < content.length && !found) {
      if (content[j] === '\n') {
        ++loc.line;
      }
      ++j;
      found = startsWith(content, IT_MARKER, j);
    }
    const unchangedContent = content.substring(i, j);
    stream.write(`\n#line ${startLine} "${path.resolve(filePath)}"\n`);
    stream.write(unchangedContent);
    if (j === content.length) {
      i = content.length;
      continue;
    }
    startLine = loc.line;
    i = j + IT_MARKER.length;
    const cppString = readCppString(content, i, filePath);
    if (cppString == null) {
      continue;
    }
    let caseName;
    [caseName, i] = cppString;
    if (content[i] !== ' ') {
      fatalError(filePath, content, i, 'expected space after @it title');
      continue;
    }
    i += 1;
    const cppBlock = readCppBlock(content, i, filePath, loc);
    if (cppBlock == null) {
      continue;
    }
    let blockContent;
    [blockContent, i] = cppBlock;
    ++caseCount;
    testFunctions.push({
      name: caseName,
      functionName: writeTestFunction(stream, caseCount, caseName, blockContent, filePath, relFilePath, startLine)
    });
  }
  stream.write('\n');
  writeMain(stream, testFunctions, filePath);
}

function updateIncludes(sourceCode, sourceDirPath, targetDirPath) {
  const lines = sourceCode.split('\n');
  return lines.map(line => {
    if (line.length === 0 || line[0] != '#') {
      return line;
    }
    const match = line.match(/#include "([^"]+)"/);
    if (match == null) {
      return line;
    }
    const includedPath = path.resolve(sourceDirPath, match[1]);
    return `#include "${path.relative(targetDirPath, includedPath)}"`;
  }).join('\n');
}

cli(function () {
  if (process.argv.length < 6) {
    reporting.fatalError(1, 'not enough arguments');
    return;
  }
  const sourcePath = process.argv[2];
  const targetPath = process.argv[3];
  const depfilePath = process.argv[4];
  const headerPath = process.argv[5];
  let targetStream, targetDirPath;
  if (targetPath === '-') {
    targetStream = process.stdout;
    targetDirPath = process.cwd();
  } else {
    targetStream = fs.createWriteStream(targetPath);
    targetDirPath = path.dirname(targetPath);
  }
  const content = updateIncludes(
    fs.readFileSync(sourcePath, 'utf8'),
    path.dirname(sourcePath),
    targetDirPath
  );
  transform(content, targetStream, sourcePath, targetDirPath, headerPath);
  if (targetPath !== '-') {
    targetStream.end();
  }
  writeNodeDepFile(depfilePath, targetPath);
});
