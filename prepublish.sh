#/bin/sh

set -ev

./configure.js --compiler clang++
dist/upd script dist/upd > bootstrap/gen_upd.sh

./configure.js --compiler g++
dist/upd script dist/upd > bootstrap/gen_upd_g++.sh
