/* @flow */

'use strict';

const crypto = require('crypto');

function hashString(caseTitle: string): string {
  return crypto.createHash('md5')
    .update(caseTitle)
    .digest('base64')
    .replace(/[/+]/g, 'a')
    .substr(0, 8);
}

module.exports = hashString;
