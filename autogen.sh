#!/bin/sh
set -e
srcdir="$(dirname $0)"

if [ -z ${LIBTOOLIZE} ] && GLIBTOOLIZE="`which glibtoolize 2>/dev/null`"; then
  LIBTOOLIZE="${GLIBTOOLIZE}"
  export LIBTOOLIZE
fi

if [ "${SKIP_AUTO_DEPS}" = "true" ]; then
    :
elif which apt-get >/dev/null 2>&1; then
    deb_list=""
    deb_list="${deb_list} python-pip python-setuptools python-dev"
    deb_list="${deb_list} python3-pip python3-setuptools python3-dev"
    deb_list="${deb_list} build-essential g++ libtool autotools-dev automake bsdmainutils pkg-config"
    deb_list="${deb_list} autoconf autoconf2.*"
    deb_list="${deb_list} libssl-dev libevent-dev"
    deb_list="${deb_list} libboost-system-dev libboost-filesystem-dev libboost-chrono-dev"
    deb_list="${deb_list} libboost-program-options-dev libboost-test-dev libboost-thread-dev"
    deb_list="${deb_list} libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools"
    deb_list="${deb_list} libprotobuf-dev protobuf-compiler"
    deb_list="${deb_list} libqrencode-dev"
    deb_list="${deb_list} libminiupnpc-dev libzmq3-dev"
    deb_list="${deb_list} libdb4.8-dev libdb4.8++-dev"
    deb_list="${deb_list} lcov default-jre-headless"
    deb_list="${deb_list} ccache"
    
    if [ "$TARGET" = "x86_64-w64-mingw32" ]; then
        deb_list="${deb_list} mingw-w64 wine64 wine-binfmt"
    fi
    
    deb_to_install=""
    for d in $deb_list; do
        if ! dpkg -s $d >/dev/null 2>&1; then
            case $d in
                libdb4.8*)
                    echo 'Please manually install libdb4.8-dev & libdb4.8++-dev'
                    echo ' from https://launchpad.net/~bitcoin/+archive/ubuntu/bitcoin/+packages'
                    exit 1
                    ;;
                *)
                    deb_to_install="${deb_to_install} ${d}"
                    ;;
            esac
        fi
    done
    
    if [ -n "${deb_to_install}" ]; then
        echo "Auto-trying to install Debian/Ubuntu deps"
        set -x
        sudo -n /usr/bin/apt-get install --no-install-recommends -y ${deb_to_install}
        set +x
    fi
    
    pip_install() {
        ( which futoin-cid && cd $srcdir && cte pip install "$@" )

        /usr/bin/pip install --user "$@"
    }

    echo 'import dash_hash' | /usr/bin/env python - >/dev/null 2>&1 || \
        pip_install git+https://github.com/dashpay/dash_hash
    pip_install pyzmq
fi

autoreconf --install --force --warnings=all $srcdir

if [ "$TARGET" = "x86_64-w64-mingw32" ]; then
    echo "Preparing Win64 deps"
    make -C $srcdir/depends HOST=x86_64-w64-mingw32 -j${MAKEJOBS:-$(nproc)}
fi

