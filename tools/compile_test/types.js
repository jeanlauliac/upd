/* @flow */

'use strict';

export type ReadChar = () => ?string;
export type Write = (string) => void;

export type Location = {|+column: number, +line: number, +index: number|};
export type LocatedChar = {|+char: ?string, +location: Location|};
export type ReadLocatedChar = () => LocatedChar;
