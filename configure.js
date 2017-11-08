#!/usr/bin/env node_modules/.bin/babel-node --plugins transform-flow-strip-types
/* @flow */

const BUILD_DIR = "gen";

const path = require('path');
const fs = require('fs');
const updfile = require('./tools/lib/updfile');

const options = {
  compilerBinary: 'clang++',
};

const argv = process.argv;
for (let i = 2; i < argv.length; ++i) {
  const arg = argv[i];
  switch (arg) {
    case '--compiler':
      if (++i === argv.length) throw new Error('`--compiler` needs value');
      options.compilerBinary = argv[i];
      break;
    default:
      throw new Error('unrecognized argument: `' + arg + '`');
  }
}

const OPTIMIZATION_FLAGS = ['-Ofast'];
if (options.compilerBinary === 'clang++') {
  OPTIMIZATION_FLAGS.push('-fno-rtti');
}

const manifest = new updfile.ManifestBuilder();

const COMMON_NATIVE_COMPILE_FLAGS = ["-Wall", '-Wextra', '-Wshadow-all', "-MMD"];
if (options.compilerBinary === 'clang++') {
  COMMON_NATIVE_COMPILE_FLAGS.push("-Werror", '-pedantic-errors');
} else {
  COMMON_NATIVE_COMPILE_FLAGS.push('-fpermissive');
}

const COMMON_CPLUSPLUS_COMPILE_FLAGS =
  COMMON_NATIVE_COMPILE_FLAGS.concat(["-std=c++14"]);
if (options.compilerBinary === 'clang++') {
  COMMON_CPLUSPLUS_COMPILE_FLAGS.push("-stdlib=libc++");
}

const compilerPath = searchInPATH(options.compilerBinary);

const compile_optimized_cpp_cli = manifest.cli_template(
  compilerPath,
  updfile.makeCli(
    ["-c", "-o", "$output_file"].concat(
      COMMON_CPLUSPLUS_COMPILE_FLAGS,
      OPTIMIZATION_FLAGS,
      ["-MF", "$dependency_file", "$input_files"],
    )
  )
);

const compile_debug_cpp_cli = manifest.cli_template(compilerPath, [
  {literals: ["-c", "-o"], variables: ["output_file"]},
  {
    literals: COMMON_CPLUSPLUS_COMPILE_FLAGS.concat(['-g', "-MF"]),
    variables: ["dependency_file"]
  },
  {literals: [], variables: ["input_files"]},
]);

const compile_optimized_c_cli = manifest.cli_template(compilerPath, [
  {literals: ["-c", "-o"], variables: ["output_file"]},
  {
    literals: COMMON_NATIVE_COMPILE_FLAGS.concat(["-x", "c"], OPTIMIZATION_FLAGS, ["-MF"]),
    variables: ["dependency_file"]
  },
  {literals: [], variables: ["input_files"]},
]);

const compile_debug_c_cli = manifest.cli_template(compilerPath, [
  {literals: ["-c", "-o"], variables: ["output_file"]},
  {
    literals: COMMON_NATIVE_COMPILE_FLAGS.concat(["-x", "c", "-g", "-MF"]),
    variables: ["dependency_file"]
  },
  {literals: [], variables: ["input_files"]},
]);

const compiled_tools = manifest.rule(
  manifest.cli_template(
    'node_modules/.bin/babel',
    updfile.makeCli([
      "--source-maps", "inline", "--plugins", "transform-flow-strip-types",
      "-o", "$output_file", "$input_files"
    ]),
  ),
  [manifest.source('(tools/**/*.js)')],
  `${BUILD_DIR}/($1)`
);

const cppt_sources = manifest.source("(src/lib/**/*).cppt");
const testingHeaderPath = path.resolve(__dirname, 'tools/lib/testing.h');

const test_cpp_files = manifest.rule(
  manifest.cli_template('node', [
    {
      literals: [`${BUILD_DIR}/tools/compile_test.js`],
      variables: ["input_files", "output_file", "dependency_file"],
    },
    {
      literals: [testingHeaderPath],
    }
  ]),
  [cppt_sources],
  `${BUILD_DIR}/($1).cpp`,
  [compiled_tools]
);

const cli_parser_cpp_file = manifest.rule(
  manifest.cli_template(`node`, [
    {
      literals: [`${BUILD_DIR}/tools/gen_cli_parser.js`],
      variables: ["output_file"]
    },
    {
      literals: [`${BUILD_DIR}/src/lib/cli/parse_options.h`],
      variables: ["dependency_file", "input_files"],
    },
  ]),
  [manifest.source("(src/lib/cli/parse_options).json")],
  `${BUILD_DIR}/($1).cpp`,
  [compiled_tools]
);

const cpp_files = [manifest.source("(src/lib/**/*).cpp"), cli_parser_cpp_file];

const compiled_optimized_cpp_files = manifest.rule(
  compile_optimized_cpp_cli,
  cpp_files,
  `${BUILD_DIR}/optimized/$1.o`,
  [cli_parser_cpp_file]
);

const compiled_debug_cpp_files = manifest.rule(
  compile_debug_cpp_cli,
  cpp_files,
  `${BUILD_DIR}/debug/$1.o`,
  [cli_parser_cpp_file]
);

const compiled_optimized_c_files = manifest.rule(
  compile_optimized_c_cli,
  [manifest.source("(src/lib/**/*).c")],
  `${BUILD_DIR}/optimized/$1.o`
);

const compiled_debug_c_files = manifest.rule(
  compile_debug_c_cli,
  [manifest.source("(src/lib/**/*).c")],
  `${BUILD_DIR}/debug/$1.o`
);

const tests_cpp_file = manifest.rule(
  manifest.cli_template('node', [
    {
      literals: [`${BUILD_DIR}/tools/index_tests.js`],
      variables: ["output_file", "dependency_file"],
    },
    {
      literals: [testingHeaderPath],
      variables: ["input_files"],
    },
  ]),
  [cppt_sources],
  `${BUILD_DIR}/(tests).cpp`,
  [compiled_tools]
);

const package_cpp_file = manifest.rule(
  manifest.cli_template('node', [
    {
      literals: [`${BUILD_DIR}/tools/gen_package_info.js`],
      variables: ["output_file", "dependency_file", "input_files"],
    }
  ]),
  [manifest.source("package.json")],
  `${BUILD_DIR}/(package).cpp`,
  [compiled_tools]
);

const compiled_optimized_main_files = manifest.rule(
  compile_optimized_cpp_cli,
  [manifest.source("(src/main).cpp"), package_cpp_file],
  `${BUILD_DIR}/optimized/$1.o`,
  [cli_parser_cpp_file]
);

const compiled_debug_main_files = manifest.rule(
  compile_debug_cpp_cli,
  [manifest.source("(src/main).cpp"), package_cpp_file],
  `${BUILD_DIR}/debug/$1.o`,
  [cli_parser_cpp_file]
);

const compiled_test_files = manifest.rule(
  compile_debug_cpp_cli,
  [manifest.source("(tools/lib/testing).cpp"), test_cpp_files, tests_cpp_file],
  `${BUILD_DIR}/debug/$1.o`,
  [cli_parser_cpp_file]
);

const commonLinkFlags = ["-Wall", "-std=c++14"];
if (options.compilerBinary === 'clang++') {
  commonLinkFlags.push("-stdlib=libc++");
}

const link_optimized_cpp_cli = manifest.cli_template(compilerPath, [
  {literals: ["-o"], variables: ["output_file"]},
  {
    literals: commonLinkFlags.concat(OPTIMIZATION_FLAGS),
    variables: ["input_files"]
  },
  {literals: ["-lpthread"]},
]);

const link_debug_cpp_cli = manifest.cli_template(compilerPath, [
  {literals: ["-o"], variables: ["output_file"]},
  {
    literals: commonLinkFlags.concat(["-g"]),
    variables: ["input_files"]
  },
  {literals: ["-lpthread"]},
]);

manifest.rule(
  manifest.cli_template('strip', [
    {literals: ['-o'], variables: ['output_file', 'input_files']},
  ]),
  [
    manifest.rule(
      link_optimized_cpp_cli,
      [compiled_optimized_cpp_files, compiled_optimized_c_files, compiled_optimized_main_files],
      `${BUILD_DIR}/pre_strip_upd`
    ),
  ],
  `dist/upd`
);

manifest.rule(
  link_debug_cpp_cli,
  [compiled_debug_cpp_files, compiled_debug_c_files, compiled_debug_main_files],
  "dist/upd_debug"
);

manifest.rule(
  link_debug_cpp_cli,
  [compiled_debug_cpp_files, compiled_debug_c_files, compiled_test_files],
  "dist/unit_tests"
);

manifest.export(__dirname);

function searchInPATH(name) {
  const {PATH} = process.env;
  if (PATH == null) {
    throw new Error('PATH environment variable is missing');
  }
  const dirPaths = PATH.split(path.delimiter);
  for (const dirPath of dirPaths) {
    const filePath = path.resolve(dirPath, name);
    if (fs.existsSync(filePath)) {
      return filePath;
    }
  }
  throw new Error(`could not resolve binary ${name}`);
}
