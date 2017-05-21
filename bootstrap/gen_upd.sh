#!/bin/bash
# generated with `upd --shell-script`
set -ev

mkdir -p .build_files/src/lib
clang++ -c -o .build_files/src/lib/xxhash64.o -std=c++14 -glldb -Wall -fcolor-diagnostics -MMD -MF /dev/null -I /usr/local/include src/lib/xxhash64.cpp
clang++ -c -o .build_files/src/lib/glob_test.o -std=c++14 -glldb -Wall -fcolor-diagnostics -MMD -MF /dev/null -I /usr/local/include src/lib/glob_test.cpp
clang++ -c -o .build_files/src/lib/substitution.o -std=c++14 -glldb -Wall -fcolor-diagnostics -MMD -MF /dev/null -I /usr/local/include src/lib/substitution.cpp
clang++ -c -o .build_files/src/lib/path_glob.o -std=c++14 -glldb -Wall -fcolor-diagnostics -MMD -MF /dev/null -I /usr/local/include src/lib/path_glob.cpp
clang++ -c -o .build_files/src/lib/io.o -std=c++14 -glldb -Wall -fcolor-diagnostics -MMD -MF /dev/null -I /usr/local/include src/lib/io.cpp
clang++ -c -o .build_files/src/lib/path.o -std=c++14 -glldb -Wall -fcolor-diagnostics -MMD -MF /dev/null -I /usr/local/include src/lib/path.cpp
clang++ -c -o .build_files/src/lib/inspect.o -std=c++14 -glldb -Wall -fcolor-diagnostics -MMD -MF /dev/null -I /usr/local/include src/lib/inspect.cpp
clang++ -c -o .build_files/src/lib/update.o -std=c++14 -glldb -Wall -fcolor-diagnostics -MMD -MF /dev/null -I /usr/local/include src/lib/update.cpp
clang++ -c -o .build_files/src/lib/glob.o -std=c++14 -glldb -Wall -fcolor-diagnostics -MMD -MF /dev/null -I /usr/local/include src/lib/glob.cpp
clang++ -c -o .build_files/src/lib/cli.o -std=c++14 -glldb -Wall -fcolor-diagnostics -MMD -MF /dev/null -I /usr/local/include src/lib/cli.cpp
clang++ -c -o .build_files/src/lib/depfile.o -std=c++14 -glldb -Wall -fcolor-diagnostics -MMD -MF /dev/null -I /usr/local/include src/lib/depfile.cpp
clang++ -c -o .build_files/src/lib/update_log.o -std=c++14 -glldb -Wall -fcolor-diagnostics -MMD -MF /dev/null -I /usr/local/include src/lib/update_log.cpp
clang++ -c -o .build_files/src/lib/command_line_template.o -std=c++14 -glldb -Wall -fcolor-diagnostics -MMD -MF /dev/null -I /usr/local/include src/lib/command_line_template.cpp
clang++ -c -o .build_files/src/lib/xxhash.o -x c -glldb -Wall -fcolor-diagnostics -MMD -MF /dev/null -I /usr/local/include src/lib/xxhash.c
mkdir -p .build_files
tools/gen_package_info.js .build_files/package.cpp /dev/null package.json
mkdir -p .build_files/src
clang++ -c -o .build_files/src/main.o -std=c++14 -glldb -Wall -fcolor-diagnostics -MMD -MF /dev/null -I /usr/local/include src/main.cpp
clang++ -c -o .build_files/package.o -std=c++14 -glldb -Wall -fcolor-diagnostics -MMD -MF /dev/null -I /usr/local/include .build_files/package.cpp
mkdir -p dist
clang++ -o dist/upd -Wall -glldb -fcolor-diagnostics -std=c++14 -L /usr/local/lib .build_files/src/lib/xxhash64.o .build_files/src/lib/glob_test.o .build_files/src/lib/substitution.o .build_files/src/lib/path_glob.o .build_files/src/lib/io.o .build_files/src/lib/path.o .build_files/src/lib/inspect.o .build_files/src/lib/update.o .build_files/src/lib/glob.o .build_files/src/lib/cli.o .build_files/src/lib/depfile.o .build_files/src/lib/update_log.o .build_files/src/lib/command_line_template.o .build_files/src/lib/xxhash.o .build_files/package.o .build_files/src/main.o
