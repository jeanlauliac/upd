#!/bin/bash
# generated with `upd --shell-script`
set -ev

mkdir -p gen/tools
node_modules/.bin/babel --source-maps inline --plugins transform-flow-strip-types -o gen/tools/compile_test.js tools/compile_test.js
node_modules/.bin/babel --source-maps inline --plugins transform-flow-strip-types -o gen/tools/gen_cli_parser.js tools/gen_cli_parser.js
node_modules/.bin/babel --source-maps inline --plugins transform-flow-strip-types -o gen/tools/gen_package_info.js tools/gen_package_info.js
node_modules/.bin/babel --source-maps inline --plugins transform-flow-strip-types -o gen/tools/index_tests.js tools/index_tests.js
mkdir -p gen/tools/lib
node_modules/.bin/babel --source-maps inline --plugins transform-flow-strip-types -o gen/tools/lib/cli.js tools/lib/cli.js
mkdir -p gen/tools/lib/__tests__
node_modules/.bin/babel --source-maps inline --plugins transform-flow-strip-types -o gen/tools/lib/__tests__/writeDepFile-test.js tools/lib/__tests__/writeDepFile-test.js
node_modules/.bin/babel --source-maps inline --plugins transform-flow-strip-types -o gen/tools/lib/reporting.js tools/lib/reporting.js
node_modules/.bin/babel --source-maps inline --plugins transform-flow-strip-types -o gen/tools/lib/updfile.js tools/lib/updfile.js
node_modules/.bin/babel --source-maps inline --plugins transform-flow-strip-types -o gen/tools/lib/writeDepFile.js tools/lib/writeDepFile.js
mkdir -p gen/optimized/src/lib
clang++ -c -o gen/optimized/src/lib/xxhash.o -Wall -fcolor-diagnostics -MMD -x c -Ofast -fno-rtti -MF /dev/null src/lib/xxhash.c
mkdir -p gen/src/lib/cli
node gen/tools/gen_cli_parser.js gen/src/lib/cli/parse_options.cpp gen/src/lib/cli/parse_options.h /dev/null src/lib/cli/parse_options.json
mkdir -p gen
node gen/tools/gen_package_info.js gen/package.cpp /dev/null package.json
mkdir -p gen/optimized/src/lib/cli
clang++ -c -o gen/optimized/src/lib/cli/parse_options.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -fno-rtti -MF /dev/null gen/src/lib/cli/parse_options.cpp
clang++ -c -o gen/optimized/src/lib/xxhash64.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -fno-rtti -MF /dev/null src/lib/xxhash64.cpp
clang++ -c -o gen/optimized/src/lib/update_plan.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -fno-rtti -MF /dev/null src/lib/update_plan.cpp
clang++ -c -o gen/optimized/src/lib/update_log.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -fno-rtti -MF /dev/null src/lib/update_log.cpp
clang++ -c -o gen/optimized/src/lib/update.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -fno-rtti -MF /dev/null src/lib/update.cpp
clang++ -c -o gen/optimized/src/lib/substitution.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -fno-rtti -MF /dev/null src/lib/substitution.cpp
clang++ -c -o gen/optimized/src/lib/path.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -fno-rtti -MF /dev/null src/lib/path.cpp
clang++ -c -o gen/optimized/src/lib/update_worker.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -fno-rtti -MF /dev/null src/lib/update_worker.cpp
clang++ -c -o gen/optimized/src/lib/path_glob.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -fno-rtti -MF /dev/null src/lib/path_glob.cpp
clang++ -c -o gen/optimized/src/lib/inspect.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -fno-rtti -MF /dev/null src/lib/inspect.cpp
clang++ -c -o gen/optimized/src/lib/manifest.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -fno-rtti -MF /dev/null src/lib/manifest.cpp
clang++ -c -o gen/optimized/src/lib/glob.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -fno-rtti -MF /dev/null src/lib/glob.cpp
clang++ -c -o gen/optimized/src/lib/gen_update_map.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -fno-rtti -MF /dev/null src/lib/gen_update_map.cpp
clang++ -c -o gen/optimized/src/lib/fd_char_reader.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -fno-rtti -MF /dev/null src/lib/fd_char_reader.cpp
clang++ -c -o gen/optimized/src/lib/io.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -fno-rtti -MF /dev/null src/lib/io.cpp
clang++ -c -o gen/optimized/src/lib/glob_test.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -fno-rtti -MF /dev/null src/lib/glob_test.cpp
clang++ -c -o gen/optimized/src/lib/depfile.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -fno-rtti -MF /dev/null src/lib/depfile.cpp
clang++ -c -o gen/optimized/src/lib/cli/parse_concurrency.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -fno-rtti -MF /dev/null src/lib/cli/parse_concurrency.cpp
clang++ -c -o gen/optimized/src/lib/execute_manifest.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -fno-rtti -MF /dev/null src/lib/execute_manifest.cpp
clang++ -c -o gen/optimized/src/lib/command_line_template.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -fno-rtti -MF /dev/null src/lib/command_line_template.cpp
clang++ -c -o gen/optimized/src/lib/command_line_runner.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -fno-rtti -MF /dev/null src/lib/command_line_runner.cpp
mkdir -p gen/optimized/src
clang++ -c -o gen/optimized/src/main.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -fno-rtti -MF /dev/null src/main.cpp
mkdir -p gen/optimized
clang++ -c -o gen/optimized/package.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -fno-rtti -MF /dev/null gen/package.cpp
clang++ -o gen/pre_strip_upd -Wall -fcolor-diagnostics -stdlib=libc++ -std=c++14 -Ofast -fno-rtti gen/optimized/src/lib/cli/parse_options.o gen/optimized/src/lib/xxhash64.o gen/optimized/src/lib/update_plan.o gen/optimized/src/lib/update_log.o gen/optimized/src/lib/update.o gen/optimized/src/lib/substitution.o gen/optimized/src/lib/path.o gen/optimized/src/lib/update_worker.o gen/optimized/src/lib/path_glob.o gen/optimized/src/lib/inspect.o gen/optimized/src/lib/manifest.o gen/optimized/src/lib/glob.o gen/optimized/src/lib/gen_update_map.o gen/optimized/src/lib/fd_char_reader.o gen/optimized/src/lib/io.o gen/optimized/src/lib/glob_test.o gen/optimized/src/lib/depfile.o gen/optimized/src/lib/cli/parse_concurrency.o gen/optimized/src/lib/execute_manifest.o gen/optimized/src/lib/command_line_template.o gen/optimized/src/lib/command_line_runner.o gen/optimized/src/lib/xxhash.o gen/optimized/package.o gen/optimized/src/main.o -lpthread
mkdir -p dist
strip -o dist/upd gen/pre_strip_upd
