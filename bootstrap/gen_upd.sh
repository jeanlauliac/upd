#!/bin/bash
# generated with `upd --shell-script`
set -ev

mkdir -p .build_files/optimized/src/lib
clang++ -c -o .build_files/optimized/src/lib/xxhash64.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -MF /dev/null src/lib/xxhash64.cpp
clang++ -c -o .build_files/optimized/src/lib/update_plan.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -MF /dev/null src/lib/update_plan.cpp
clang++ -c -o .build_files/optimized/src/lib/update_log.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -MF /dev/null src/lib/update_log.cpp
clang++ -c -o .build_files/optimized/src/lib/update.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -MF /dev/null src/lib/update.cpp
clang++ -c -o .build_files/optimized/src/lib/substitution.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -MF /dev/null src/lib/substitution.cpp
clang++ -c -o .build_files/optimized/src/lib/path_glob.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -MF /dev/null src/lib/path_glob.cpp
clang++ -c -o .build_files/optimized/src/lib/manifest.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -MF /dev/null src/lib/manifest.cpp
clang++ -c -o .build_files/optimized/src/lib/io.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -MF /dev/null src/lib/io.cpp
clang++ -c -o .build_files/optimized/src/lib/path.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -MF /dev/null src/lib/path.cpp
clang++ -c -o .build_files/optimized/src/lib/inspect.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -MF /dev/null src/lib/inspect.cpp
clang++ -c -o .build_files/optimized/src/lib/glob_test.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -MF /dev/null src/lib/glob_test.cpp
clang++ -c -o .build_files/optimized/src/lib/gen_update_map.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -MF /dev/null src/lib/gen_update_map.cpp
clang++ -c -o .build_files/optimized/src/lib/execute_manifest.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -MF /dev/null src/lib/execute_manifest.cpp
clang++ -c -o .build_files/optimized/src/lib/fd_char_reader.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -MF /dev/null src/lib/fd_char_reader.cpp
clang++ -c -o .build_files/optimized/src/lib/cli.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -MF /dev/null src/lib/cli.cpp
clang++ -c -o .build_files/optimized/src/lib/command_line_template.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -MF /dev/null src/lib/command_line_template.cpp
clang++ -c -o .build_files/optimized/src/lib/depfile.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -MF /dev/null src/lib/depfile.cpp
clang++ -c -o .build_files/optimized/src/lib/glob.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -MF /dev/null src/lib/glob.cpp
clang++ -c -o .build_files/optimized/src/lib/command_line_runner.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -MF /dev/null src/lib/command_line_runner.cpp
clang++ -c -o .build_files/optimized/src/lib/xxhash.o -Wall -fcolor-diagnostics -MMD -x c -Ofast -MF /dev/null src/lib/xxhash.c
mkdir -p .build_files
tools/gen_package_info.js .build_files/package.cpp /dev/null package.json
mkdir -p .build_files/optimized/src
clang++ -c -o .build_files/optimized/src/main.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -MF /dev/null src/main.cpp
mkdir -p .build_files/optimized
clang++ -c -o .build_files/optimized/package.o -Wall -fcolor-diagnostics -MMD -std=c++14 -stdlib=libc++ -Ofast -MF /dev/null .build_files/package.cpp
mkdir -p dist
clang++ -o dist/upd -Wall -fcolor-diagnostics -stdlib=libc++ -std=c++14 -Ofast .build_files/optimized/src/lib/xxhash64.o .build_files/optimized/src/lib/update_plan.o .build_files/optimized/src/lib/update_log.o .build_files/optimized/src/lib/update.o .build_files/optimized/src/lib/substitution.o .build_files/optimized/src/lib/path_glob.o .build_files/optimized/src/lib/manifest.o .build_files/optimized/src/lib/io.o .build_files/optimized/src/lib/path.o .build_files/optimized/src/lib/inspect.o .build_files/optimized/src/lib/glob_test.o .build_files/optimized/src/lib/gen_update_map.o .build_files/optimized/src/lib/execute_manifest.o .build_files/optimized/src/lib/fd_char_reader.o .build_files/optimized/src/lib/cli.o .build_files/optimized/src/lib/command_line_template.o .build_files/optimized/src/lib/depfile.o .build_files/optimized/src/lib/glob.o .build_files/optimized/src/lib/command_line_runner.o .build_files/optimized/src/lib/xxhash.o .build_files/optimized/package.o .build_files/optimized/src/main.o -lpthread
