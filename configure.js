#!/usr/bin/env node

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

const BUILD_DIR = ".build_files";
const OPTIMIZATION_FLAG = '-Ofast';

const manifest = new updfile.Manifest();

const common_compile_flags = ["-Wall", "-fcolor-diagnostics", "-MMD"];
const common_cpp_compile_flags = common_compile_flags.concat(["-std=c++14", "-stdlib=libc++"]);

const compile_optimized_cpp_cli = manifest.cli_template(options.compilerBinary, [
  {literals: ["-c", "-o"], variables: ["output_file"]},
  {
    literals: common_cpp_compile_flags.concat([OPTIMIZATION_FLAG, "-MF"]),
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
    literals: common_compile_flags.concat(["-x", "c", OPTIMIZATION_FLAG, "-MF"]),
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

const cppt_sources = manifest.source("(src/lib/**/*).cppt");

const test_cpp_files = manifest.rule(
  manifest.cli_template('tools/compile_test.js', [
    {variables: ["input_files", "output_file", "dependency_file"]},
  ]),
  [cppt_sources],
  `${BUILD_DIR}/($1).cpp`
);

const cpp_files = manifest.source("(src/lib/**/*).cpp");

const compiled_optimized_cpp_files = manifest.rule(
  compile_optimized_cpp_cli,
  [cpp_files],
  `${BUILD_DIR}/optimized/$1.o`
);

const compiled_debug_cpp_files = manifest.rule(
  compile_debug_cpp_cli,
  [cpp_files],
  `${BUILD_DIR}/debug/$1.o`
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
  manifest.cli_template('tools/index_tests.js', [
    {variables: ["output_file", "dependency_file", "input_files"]}
  ]),
  [cppt_sources],
  `${BUILD_DIR}/(tests).cpp`
);

const package_cpp_file = manifest.rule(
  manifest.cli_template('tools/gen_package_info.js', [
    {variables: ["output_file", "dependency_file", "input_files"]}
  ]),
  [manifest.source("package.json")],
  `${BUILD_DIR}/(package).cpp`
);

const compiled_optimized_main_files = manifest.rule(
  compile_optimized_cpp_cli,
  [manifest.source("(src/main).cpp"), package_cpp_file],
  `${BUILD_DIR}/optimized/$1.o`
);

const compiled_debug_main_files = manifest.rule(
  compile_debug_cpp_cli,
  [manifest.source("(src/main).cpp"), package_cpp_file],
  `${BUILD_DIR}/debug/$1.o`
);

const compiled_test_files = manifest.rule(
  compile_debug_cpp_cli,
  [manifest.source("(tools/lib/testing).cpp"), test_cpp_files, tests_cpp_file],
  `${BUILD_DIR}/debug/$1.o`
);

const commonLinkFlags = ["-Wall", "-fcolor-diagnostics", "-stdlib=libc++", "-std=c++14"];

const link_optimized_cpp_cli = manifest.cli_template(options.compilerBinary, [
  {literals: ["-o"], variables: ["output_file"]},
  {
    literals: commonLinkFlags.concat([OPTIMIZATION_FLAG]),
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
  link_optimized_cpp_cli,
  [compiled_optimized_cpp_files, compiled_optimized_c_files, compiled_optimized_main_files],
  "dist/upd"
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
