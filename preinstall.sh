#/bin/sh

set -e

rm -f bootstrap/upd

if [ "$(uname)" == "Darwin" ]; then
  ln -s upd_darwin bootstrap/upd
else
  ln -s upd_linux bootstrap/upd
fi
