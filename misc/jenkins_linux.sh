#!/bin/bash

#------------------------------------------------------
# This is actual Jenkins build script for Linux target
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
  export CC=clang-5.0
  export CXX=clang++-5.0
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
  echo "Using legacy build approach build"

  rm -fr "${WORKSPACE}/build"

  if [[ "${CLEAN:-}" == "true" ]]; then
      git clean -fdx .
      ./autogen.sh
  fi

  if [[ "${CLEAN:-}" == "true" || "${RECONFIGURE:-}" == "true" ]]; then
      CC=clang-5.0 CXX=clang++-5.0 CFLAGS="-Wall" CXXFLAGS="-Wall" ./configure --prefix="${WORKSPACE}/build/${energi_version}" --with-gui
  fi

  make -j 2
  make install
  
  if [[ "${RUN_TESTS:-}" == "true" ]]; then
      rm -rf "${WORKSPACE}/test_results"
      mkdir -p "${WORKSPACE}/test_results" 2> /dev/null
      pushd "${WORKSPACE}/build/${energi_version}/bin"
          ./test_energi --log_format=XML --log_level=warning --log_sink="${WORKSPACE}/test_results/results.xml" --report_format=XML --report_level=detailed --report_sink="${WORKSPACE}/test_results/report.xml" --result_code=no
          ./test_energi-qt -xml -o "${WORKSPACE}/test_results/test_energi-qt.xml"
      popd
  fi
  
  pushd "${WORKSPACE}/build"
      tar cvzf "${energi_version}.tar.gz" "${energi_version}"
  popd
fi

