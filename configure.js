#!/usr/bin/env node

const updfile = require('./.build_files/tools/lib/updfile');
const path = require('path');

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

const BUILD_DIR = ".build_files";
const OPTIMIZATION_FLAGS = ['-Ofast', '-fno-rtti'];

const manifest = new updfile.ManifestBuilder();

const common_compile_flags = ["-Wall", "-fcolor-diagnostics", "-MMD"];
const common_cpp_compile_flags = common_compile_flags.concat(["-std=c++14", "-stdlib=libc++"]);

const compile_js_cli = manifest.cli_template('node_modules/.bin/babel', [
  {
    literals: ["--plugins", "transform-flow-strip-types", "-o"],
    variables: ["output_file", "input_files"],
  },
]);

const compile_optimized_cpp_cli = manifest.cli_template(options.compilerBinary, [
  {literals: ["-c", "-o"], variables: ["output_file"]},
  {
    literals: common_cpp_compile_flags.concat(OPTIMIZATION_FLAGS, ["-MF"]),
    variables: ["dependency_file"]
  },
  {literals: [], variables: ["input_files"]},
]);

const compile_debug_cpp_cli = manifest.cli_template(options.compilerBinary, [
  {literals: ["-c", "-o"], variables: ["output_file"]},
  {
    literals: common_cpp_compile_flags.concat(['-g', "-MF"]),
    variables: ["dependency_file"]
  },
  {literals: [], variables: ["input_files"]},
]);

const compile_optimized_c_cli = manifest.cli_template(options.compilerBinary, [
  {literals: ["-c", "-o"], variables: ["output_file"]},
  {
    literals: common_compile_flags.concat(["-x", "c"], OPTIMIZATION_FLAGS, ["-MF"]),
    variables: ["dependency_file"]
  },
  {literals: [], variables: ["input_files"]},
]);

const compile_debug_c_cli = manifest.cli_template(options.compilerBinary, [
  {literals: ["-c", "-o"], variables: ["output_file"]},
  {
    literals: common_compile_flags.concat(["-x", "c", "-g", "-MF"]),
    variables: ["dependency_file"]
  },
  {literals: [], variables: ["input_files"]},
]);

const compiled_tools = manifest.rule(
  compile_js_cli,
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

const commonLinkFlags = ["-Wall", "-fcolor-diagnostics", "-stdlib=libc++", "-std=c++14"];

const link_optimized_cpp_cli = manifest.cli_template(options.compilerBinary, [
  {literals: ["-o"], variables: ["output_file"]},
  {
    literals: commonLinkFlags.concat(OPTIMIZATION_FLAGS),
    variables: ["input_files"]
  },
  {literals: ["-lpthread"]},
]);

const link_debug_cpp_cli = manifest.cli_template(options.compilerBinary, [
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
