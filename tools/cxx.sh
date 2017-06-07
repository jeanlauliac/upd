#/bin/sh

if [ -z "$CXX" ]; then
  echo "CXX is not defined, cannot compile!"
  exit 1
fi

$CXX $@ $CXXFLAGS
