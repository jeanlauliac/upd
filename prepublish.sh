#/bin/sh

set -ev

./configure.js --compiler clang++
dist/upd dist/upd --shell-script > bootstrap/gen_upd.sh
