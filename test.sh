#/bin/sh

set -ev

node_modules/.bin/flow
bootstrap/upd update --all
node_modules/.bin/jest
dist/unit_tests | node_modules/.bin/faucet
e2e_tests/run.js
./check_clang_format.sh
