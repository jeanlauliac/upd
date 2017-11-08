#/bin/sh

set -ev

./check_clang_format.sh
node_modules/.bin/flow
bootstrap/upd_linux update gen/tools/lib/__tests__/writeDepFile-test.js
node_modules/.bin/jest
bootstrap/upd_linux update dist/unit_tests
dist/unit_tests | node_modules/.bin/faucet
e2e_tests/run.js
