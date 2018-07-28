/* @flow */

'use strict';

const fs = require('fs');
const path = require('path');

type CliTemplateArgPath = {
  literals?: Array<string>,
  variables?: Array<string>,
};

type CliTemplate = {
  binary_path: string,
  arguments: Array<CliTemplateArgPath>,
};

type CliTemplateRef = {cli_ix: number};
type SourceRef = {source_ix: number};
type RuleRef = {rule_ix: number};
type InputRef = SourceRef | RuleRef;

type Manifest = {
  command_line_templates: Array<CliTemplate>,
  rules: Array<{
    command_line_ix: number,
    dependencies: Array<InputRef>,
    inputs: Array<InputRef>,
    output: string,
  }>,
  source_patterns: Array<string>,
};

class ManifestBuilder {
  _result: Manifest;

  constructor() {
    this._result = {
      command_line_templates: [],
      source_patterns: [],
      rules: [],
    };
  }

  cli_template(
    binary_path: string,
    args: Array<CliTemplateArgPath>,
    env?: {[string]: string},
  ): CliTemplateRef {
    const tpl: {
      binary_path: string,
      arguments: Array<CliTemplateArgPath>,
      environment?: {[string]: string}
    } = {
      binary_path,
      arguments: args,
    };
    if (env != null) {
      tpl.environment = env;
    }
    this._result.command_line_templates.push(tpl);
    return {cli_ix: this._result.command_line_templates.length - 1};
  }

  source(pattern: string): SourceRef {
    this._result.source_patterns.push(pattern);
    return {source_ix: this._result.source_patterns.length - 1};
  }

  rule(
    cli_template: CliTemplateRef,
    inputs: Array<InputRef>,
    output_pattern: string,
    order_only_dependencies?: Array<InputRef>,
    dependencies?: Array<InputRef>,
  ): RuleRef {
    const ruleObj: any = {
      command_line_ix: cli_template.cli_ix,
      inputs: inputs,
      output: output_pattern,
    };
    if (order_only_dependencies != null) {
      ruleObj.order_only_dependencies = order_only_dependencies;
    }
    if (dependencies != null) {
      ruleObj.dependencies = dependencies;
    }
    this._result.rules.push(ruleObj);
    return {rule_ix: this._result.rules.length - 1};
  }

  export(dirname: string) {
    fs.writeFileSync(
      path.resolve(dirname, 'updfile.json'),
      JSON.stringify(this._result, null, 2),
      'utf8'
    );
  }
};

function makeCli(flatPlats: Array<string>): Array<CliTemplateArgPath> {
  let curPart = {literals: [], variables: []};
  const bigParts = [curPart];
  for (const part of flatPlats) {
    if (part[0] === '$') {
      curPart.variables.push(part.slice(1));
      continue;
    }
    if (curPart.variables.length > 0) {
      curPart = {literals: [], variables: []};
      bigParts.push(curPart);
    }
    curPart.literals.push(part);
  }
  return bigParts;
}

module.exports = {ManifestBuilder, makeCli};
