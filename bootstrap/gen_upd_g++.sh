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
node_modules/.bin/babel --source-maps inline --plugins transform-flow-strip-types -o gen/tools/lib/writeNodeDepFile.js tools/lib/writeNodeDepFile.js
mkdir -p gen/tools/lib/__tests__
node_modules/.bin/babel --source-maps inline --plugins transform-flow-strip-types -o gen/tools/lib/__tests__/writeDepFile-test.js tools/lib/__tests__/writeDepFile-test.js
node_modules/.bin/babel --source-maps inline --plugins transform-flow-strip-types -o gen/tools/lib/reporting.js tools/lib/reporting.js
node_modules/.bin/babel --source-maps inline --plugins transform-flow-strip-types -o gen/tools/lib/updfile.js tools/lib/updfile.js
node_modules/.bin/babel --source-maps inline --plugins transform-flow-strip-types -o gen/tools/lib/writeDepFile.js tools/lib/writeDepFile.js
mkdir -p gen/optimized/src/lib
/usr/bin/g++ -c -o gen/optimized/src/lib/xxhash.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -x c -Ofast -MF /dev/null src/lib/xxhash.c
mkdir -p gen/src/lib/cli
node gen/tools/gen_cli_parser.js gen/src/lib/cli/parse_options.cpp gen/src/lib/cli/parse_options.h /dev/null src/lib/cli/parse_options.json
mkdir -p gen
node gen/tools/gen_package_info.js gen/package.cpp /dev/null package.json
mkdir -p gen/optimized/src/lib/cli
/usr/bin/g++ -c -o gen/optimized/src/lib/cli/parse_options.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null gen/src/lib/cli/parse_options.cpp
/usr/bin/g++ -c -o gen/optimized/src/lib/update_plan.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null src/lib/update_plan.cpp
/usr/bin/g++ -c -o gen/optimized/src/lib/update_log.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null src/lib/update_log.cpp
/usr/bin/g++ -c -o gen/optimized/src/lib/update.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null src/lib/update.cpp
mkdir -p gen/optimized/src/lib/system
/usr/bin/g++ -c -o gen/optimized/src/lib/system/spawn.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null src/lib/system/spawn.cpp
/usr/bin/g++ -c -o gen/optimized/src/lib/substitution.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null src/lib/substitution.cpp
/usr/bin/g++ -c -o gen/optimized/src/lib/xxhash64.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null src/lib/xxhash64.cpp
/usr/bin/g++ -c -o gen/optimized/src/lib/run_command_line.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null src/lib/run_command_line.cpp
/usr/bin/g++ -c -o gen/optimized/src/lib/path.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null src/lib/path.cpp
/usr/bin/g++ -c -o gen/optimized/src/lib/update_worker.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null src/lib/update_worker.cpp
/usr/bin/g++ -c -o gen/optimized/src/lib/path_glob.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null src/lib/path_glob.cpp
/usr/bin/g++ -c -o gen/optimized/src/lib/inspect.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null src/lib/inspect.cpp
/usr/bin/g++ -c -o gen/optimized/src/lib/manifest.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null src/lib/manifest.cpp
/usr/bin/g++ -c -o gen/optimized/src/lib/glob.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null src/lib/glob.cpp
/usr/bin/g++ -c -o gen/optimized/src/lib/gen_update_map.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null src/lib/gen_update_map.cpp
/usr/bin/g++ -c -o gen/optimized/src/lib/io.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null src/lib/io.cpp
/usr/bin/g++ -c -o gen/optimized/src/lib/glob_test.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null src/lib/glob_test.cpp
/usr/bin/g++ -c -o gen/optimized/src/lib/depfile.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null src/lib/depfile.cpp
/usr/bin/g++ -c -o gen/optimized/src/lib/cli/parse_concurrency.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null src/lib/cli/parse_concurrency.cpp
/usr/bin/g++ -c -o gen/optimized/src/lib/execute_manifest.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null src/lib/execute_manifest.cpp
/usr/bin/g++ -c -o gen/optimized/src/lib/command_line_template.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null src/lib/command_line_template.cpp
/usr/bin/g++ -c -o gen/optimized/src/lib/fd_char_reader.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null src/lib/fd_char_reader.cpp
/usr/bin/g++ -c -o gen/optimized/src/lib/cli/utils.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null src/lib/cli/utils.cpp
mkdir -p gen/optimized/src
/usr/bin/g++ -c -o gen/optimized/src/main.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null src/main.cpp
mkdir -p gen/optimized
/usr/bin/g++ -c -o gen/optimized/package.o -Wall -Wextra -Wshadow-all -MMD -fpermissive -std=c++14 -Ofast -MF /dev/null gen/package.cpp
/usr/bin/g++ -o gen/pre_strip_upd -Wall -std=c++14 -Ofast gen/optimized/src/lib/cli/parse_options.o gen/optimized/src/lib/update_plan.o gen/optimized/src/lib/update_log.o gen/optimized/src/lib/update.o gen/optimized/src/lib/system/spawn.o gen/optimized/src/lib/substitution.o gen/optimized/src/lib/xxhash64.o gen/optimized/src/lib/run_command_line.o gen/optimized/src/lib/path.o gen/optimized/src/lib/update_worker.o gen/optimized/src/lib/path_glob.o gen/optimized/src/lib/inspect.o gen/optimized/src/lib/manifest.o gen/optimized/src/lib/glob.o gen/optimized/src/lib/gen_update_map.o gen/optimized/src/lib/io.o gen/optimized/src/lib/glob_test.o gen/optimized/src/lib/depfile.o gen/optimized/src/lib/cli/parse_concurrency.o gen/optimized/src/lib/execute_manifest.o gen/optimized/src/lib/command_line_template.o gen/optimized/src/lib/fd_char_reader.o gen/optimized/src/lib/cli/utils.o gen/optimized/src/lib/xxhash.o gen/optimized/package.o gen/optimized/src/main.o -lpthread
mkdir -p dist
/usr/bin/strip -o dist/upd gen/pre_strip_upd