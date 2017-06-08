#/bin/sh

set -ev

./configure.js --compiler clang++
dist/upd dist/upd --shell-script > bootstrap/gen_upd.sh

./configure.js --compiler clang++-3.9
dist/upd dist/upd --shell-script > bootstrap/gen_upd_for_travis.sh
