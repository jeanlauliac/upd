#!/bin/bash

files=$(echo src/**/*.cpp src/**/*.h)
diff <(cat $files) <(node_modules/.bin/clang-format $files)
if [ $? -ne 0 ]; then
  echo 'error: code not properly formatted; run `yarn format`'
  exit 1
fi
