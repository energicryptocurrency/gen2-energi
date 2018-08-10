#!/bin/bash

echo "Current Working directory is \"${PWD}\""

#cd "${HOME}/github/energicryptocurrency/energi"
rm -fr "${WORKSPACE}/build"

energi_version="energi-PR${ghprbPullId}-${ghprbActualCommit}"

if [[ "${ghprbActualCommit}" == "develop" ]]; then
    energi_version="develop"
fi

if [[ "${CLEAN:-}" == "true" ]]; then
    git clean -fdx .
fi

pushd "depends" || exit 1
    make HOST=x86_64-w64-mingw32 -j 2 || exit 1
popd

if [[ "${CLEAN:-}" == "true" || "${RECONFIGURE:-}" == "true" ]]; then
	./autogen.sh
   ./configure --prefix="${WORKSPACE}/depends/x86_64-w64-mingw32" --with-gui || exit 1
fi

make -j 2 || exit 1

make deploy || exit 1

