#!/bin/bash

#------------------------------------------------------
# This is actual Jenkins build script for Windows target
#------------------------------------------------------

set -e

echo "Current Working directory is \"${PWD}\""

# Determine version version
if [ -n "${ghprbPullId}" ]; then
  # Original logic
  energi_version="energi-PR${ghprbPullId}-${ghprbActualCommit}"

  if [[ "${ghprbActualCommit}" == "develop" ]]; then
      energi_version="energi"
  fi
else
  energi_version="energi"
fi

if [ -e "${WORKSPACE}/futoin.json" ]; then
    echo "Using FutoIn CID-based build"

    echo "Ensuring the latest FutoIn CID is installed"
    export PATH="${PATH}:${HOME}/.local/bin"
    pip install -U --user futoin-cid

    #export SKIP_AUTO_DEPS=true
    export ENERGI_BUILD_DIR=$(pwd)/build/${energi_version}
    export ENERGI_VER=${energi_version}
    export TARGET=x86_64-w64-mingw32
    export CC=x86_64-w64-mingw32-gcc
    export CXX=x86_64-w64-mingw32-g++
    # TBD: use auto-detected CPU count?
    export MAKEJOBS=2

    if [ "${CLEAN}" = "true" -o "${RECONFIGURE}" = "true" -o "${RUN_TESTS}" = "true" ]; then
      echo "Ignoring CLEAN, RECONFIGURE and RUN_TESTS parameters!"
    fi

    cid run clean
    cid prepare
    cid build
    cid check
    cid package
else

    if [[ "${CLEAN:-}" == "true" ]]; then
        git clean -fdx .
    fi

    make -C depends HOST=x86_64-w64-mingw32 -j 2

    if [[ "${CLEAN:-}" == "true" || "${RECONFIGURE:-}" == "true" ]]; then
	    ./autogen.sh
       ./configure --prefix="${WORKSPACE}/depends/x86_64-w64-mingw32" --with-gui
    fi

    make -j 2

    make deploy
fi

