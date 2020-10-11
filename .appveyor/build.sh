#!/bin/bash
set -e
set -x

mingw32-make CXXFLAGS="-O2" LDFLAGS="-static -Wl,--as-needed"

