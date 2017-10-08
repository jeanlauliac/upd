#/bin/sh

set -ev

node_modules/.bin/flow
dist/upd update dist/unit_tests
node_modules/.bin/jest
dist/unit_tests | node_modules/.bin/faucet && e2e_tests/run.js
