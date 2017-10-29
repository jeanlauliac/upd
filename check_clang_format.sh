#!/bin/bash

files=$(find -E src -regex '.*\.(cpp|h|cppt)')
diff <(cat $files) <(node_modules/.bin/clang-format $files)
if [ $? -ne 0 ]; then
  echo 'error: code not properly formatted; run `yarn format`'
  exit 1
fi
