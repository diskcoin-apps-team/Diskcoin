#!/bin/bash
wdir=$( cd $( dirname ${BASH_SOURCE[0]} ) && pwd )
cd $wdir
echo "work dir = $wdir"

./build_osx.sh
./build_win.sh
./build_linux.sh
