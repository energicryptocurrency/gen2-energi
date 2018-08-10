#!/bin/bash

#------------------------------------------------------
# This is legacy Jenkins build script for Linux target
#------------------------------------------------------

echo "Current Working directory is \"${PWD}\""

rm -fr "${WORKSPACE}/build"

energi_version="energi-PR${ghprbPullId}-${ghprbActualCommit}"

if [[ "${ghprbActualCommit}" == "develop" ]]; then
    energi_version="energi"
fi

if [[ "${CLEAN:-}" == "true" ]]; then
    git clean -fdx .
	./autogen.sh || exit 1
fi

if [[ "${CLEAN:-}" == "true" || "${RECONFIGURE:-}" == "true" ]]; then
    CC=clang-5.0 CXX=clang++-5.0 CFLAGS="-Wall" CXXFLAGS="-Wall" ./configure --prefix="${WORKSPACE}/build/${energi_version}" --with-gui || exit 1
fi

make -j 2 || exit 1

make install || exit 1

retVal=0
if [[ "${RUN_TESTS:-}" == "true" ]]; then
	rm -rf "${WORKSPACE}/test_results"
    mkdir -p "${WORKSPACE}/test_results" 2> /dev/null
    pushd "${WORKSPACE}/build/${energi_version}/bin" || exit 1
        ./test_energi --log_format=xml --log_level=warning --log_sink="${WORKSPACE}/test_results/results.xml" --report_format=xml --report_level=detailed --report_sink="${WORKSPACE}/test_results/report.xml" --result_code=no
        ./test_energi-qt -xml -o "${WORKSPACE}/test_results/test_energi-qt.xml"
    popd || exit 1
fi

pushd "${WORKSPACE}/build" || exit 1
    tar cvzf "${energi_version}.tar.gz" "${energi_version}" || retVal=1
popd

exit ${retVal}
