'use strict';

const fs = require('fs');
const path = require('path');

class Manifest {
  constructor() {
    this._result = {
      command_line_templates: [],
      source_patterns: [],
      rules: [],
    };
  }

  cli_template(binary_path, args) {
    this._result.command_line_templates.push({
      binary_path,
      arguments: args,
    });
    return {cli_ix: this._result.command_line_templates.length - 1};
  }

  source(pattern) {
    this._result.source_patterns.push(pattern);
    return {source_ix: this._result.source_patterns.length - 1};
  }

  rule(cli_template, inputs, output_pattern, dependencies) {
    this._result.rules.push({
      dependencies: dependencies || [],
      command_line_ix: cli_template.cli_ix,
      inputs: inputs,
      output: output_pattern,
    });
    return {rule_ix: this._result.rules.length - 1};
  }

  export(dirname) {
    fs.writeFileSync(
      path.resolve(dirname, 'updfile.json'),
      JSON.stringify(this._result, null, 2),
      'utf8'
    );
  }
};

module.exports = {Manifest};
