DEPRECATED
==========

This manual is deprecated, please see consistent development environment
instructions in [energi-docs/devenv.md](../energi-docs/devenv.md).

# WINDOWS BUILD NOTES

Some notes on how to build Energi Core for Windows.

Most developers use cross-compilation from Ubuntu to build executables for
Windows. This is also used to build the release binaries.

Building on Windows itself is possible (for example using msys / mingw-w64),
but no one documented the steps to do this. If you are doing this, please contribute them.

## Cross-compilation

These steps can be performed on, for example, an Ubuntu VM. The depends system
will also work on other Linux distributions, however the commands for
installing the toolchain will be different.

### First install the toolchains:

    sudo apt-get install g++-mingw-w64-i686 mingw-w64-i686-dev g++-mingw-w64-x86-64 mingw-w64-x86-64-dev

### Ubuntu Note

MinGW must be put into POSIX mode to allow for cross compilation of Energi Core. Run the following command,
and then select the `x86_64-w64-mingw32-g++-posix` option for the default MinGW.

    sudo update-alternatives --config x86_64-w64-mingw32-g++

    There are 2 choices for the alternative x86_64-w64-mingw32-g++ (providing /usr/bin/x86_64-w64-mingw32-g++).

      Selection    Path                                   Priority   Status
    ------------------------------------------------------------
      0            /usr/bin/x86_64-w64-mingw32-g++-win32   60        auto mode
    * 1            /usr/bin/x86_64-w64-mingw32-g++-posix   30        manual mode
      2            /usr/bin/x86_64-w64-mingw32-g++-win32   60        manual mode

### To build executables for Windows 32-bit:

    cd depends
    make HOST=i686-w64-mingw32 -j4
    cd ..
    ./configure --prefix=`pwd`/depends/i686-w64-mingw32
    make

### To build executables for Windows 64-bit:

    cd depends
    make HOST=x86_64-w64-mingw32 -j4
    cd ..
    ./configure --prefix=`pwd`/depends/x86_64-w64-mingw32
    make

For further documentation on the depends system see [README.md](../depends/README.md) in the depends directory.

