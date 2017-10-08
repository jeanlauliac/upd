/* @flow */

'use strict';

declare var it: any;
declare var expect: any;

const writeDepFile = require('../writeDepFile');
const {Writable} = require('stream');

class StringWritable extends Writable {
  str: string;

  constructor() {
    super({
      write(chunk, enc, cb) {
        this.str += chunk.toString('utf8');
        cb();
      },
    });
    this.str = '';
  }
}

it('writes in a valid depfile format', () => {
  const s = new StringWritable();
  writeDepFile(s, [
    {
      targets: ["dist/a.out"],
      dependencies: ["src/main.c", "src/main.h", "src/foo.h"]
    },
  ]);
  expect(s.str).toEqual("dist/a.out: src/main.c src/main.h src/foo.h\n")
});

it('properly escapes spaces and colon characters', () => {
  const s = new StringWritable();
  writeDepFile(s, [
    {
      targets: ["dist/a.out"],
      dependencies: ["someting with spaces:foo.cpp"]
    },
  ]);
  expect(s.str).toEqual("dist/a.out: someting\\\\ with\\\\ spaces\\\\:foo.cpp\n")
});
