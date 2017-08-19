#!/bin/sh

set -e

autoreconf --install -v
./configure
make distcheck
