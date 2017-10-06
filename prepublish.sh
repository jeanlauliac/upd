#/bin/sh

set -ev

./configure.js --compiler clang++
dist/upd script dist/upd > bootstrap/gen_upd.sh
