#!/bin/bash
# generated with `upd --shell-script`
set -ev

mkdir -p .build_files/src/lib
clang++-3.9 -c -o .build_files/src/lib/xxhash64.o -std=c++14 -g -Wall -fcolor-diagnostics -stdlib=libc++ -MMD -MF /dev/null src/lib/xxhash64.cpp
clang++-3.9 -c -o .build_files/src/lib/glob_test.o -std=c++14 -g -Wall -fcolor-diagnostics -stdlib=libc++ -MMD -MF /dev/null src/lib/glob_test.cpp
clang++-3.9 -c -o .build_files/src/lib/substitution.o -std=c++14 -g -Wall -fcolor-diagnostics -stdlib=libc++ -MMD -MF /dev/null src/lib/substitution.cpp
clang++-3.9 -c -o .build_files/src/lib/path_glob.o -std=c++14 -g -Wall -fcolor-diagnostics -stdlib=libc++ -MMD -MF /dev/null src/lib/path_glob.cpp
clang++-3.9 -c -o .build_files/src/lib/io.o -std=c++14 -g -Wall -fcolor-diagnostics -stdlib=libc++ -MMD -MF /dev/null src/lib/io.cpp
clang++-3.9 -c -o .build_files/src/lib/path.o -std=c++14 -g -Wall -fcolor-diagnostics -stdlib=libc++ -MMD -MF /dev/null src/lib/path.cpp
clang++-3.9 -c -o .build_files/src/lib/inspect.o -std=c++14 -g -Wall -fcolor-diagnostics -stdlib=libc++ -MMD -MF /dev/null src/lib/inspect.cpp
clang++-3.9 -c -o .build_files/src/lib/update.o -std=c++14 -g -Wall -fcolor-diagnostics -stdlib=libc++ -MMD -MF /dev/null src/lib/update.cpp
clang++-3.9 -c -o .build_files/src/lib/glob.o -std=c++14 -g -Wall -fcolor-diagnostics -stdlib=libc++ -MMD -MF /dev/null src/lib/glob.cpp
clang++-3.9 -c -o .build_files/src/lib/cli.o -std=c++14 -g -Wall -fcolor-diagnostics -stdlib=libc++ -MMD -MF /dev/null src/lib/cli.cpp
clang++-3.9 -c -o .build_files/src/lib/depfile.o -std=c++14 -g -Wall -fcolor-diagnostics -stdlib=libc++ -MMD -MF /dev/null src/lib/depfile.cpp
clang++-3.9 -c -o .build_files/src/lib/update_log.o -std=c++14 -g -Wall -fcolor-diagnostics -stdlib=libc++ -MMD -MF /dev/null src/lib/update_log.cpp
clang++-3.9 -c -o .build_files/src/lib/command_line_template.o -std=c++14 -g -Wall -fcolor-diagnostics -stdlib=libc++ -MMD -MF /dev/null src/lib/command_line_template.cpp
clang++-3.9 -c -o .build_files/src/lib/xxhash.o -x c -g -Wall -fcolor-diagnostics -MMD -MF /dev/null src/lib/xxhash.c
mkdir -p .build_files
tools/gen_package_info.js .build_files/package.cpp /dev/null package.json
mkdir -p .build_files/src
clang++-3.9 -c -o .build_files/src/main.o -std=c++14 -g -Wall -fcolor-diagnostics -stdlib=libc++ -MMD -MF /dev/null src/main.cpp
clang++-3.9 -c -o .build_files/package.o -std=c++14 -g -Wall -fcolor-diagnostics -stdlib=libc++ -MMD -MF /dev/null .build_files/package.cpp
mkdir -p dist
clang++-3.9 -o dist/upd -Wall -g -fcolor-diagnostics -stdlib=libc++ -std=c++14 .build_files/src/lib/xxhash64.o .build_files/src/lib/glob_test.o .build_files/src/lib/substitution.o .build_files/src/lib/path_glob.o .build_files/src/lib/io.o .build_files/src/lib/path.o .build_files/src/lib/inspect.o .build_files/src/lib/update.o .build_files/src/lib/glob.o .build_files/src/lib/cli.o .build_files/src/lib/depfile.o .build_files/src/lib/update_log.o .build_files/src/lib/command_line_template.o .build_files/src/lib/xxhash.o .build_files/package.o .build_files/src/main.o -lpthread
