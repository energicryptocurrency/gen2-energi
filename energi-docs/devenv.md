
# Energi Development Environment

## Purpose

Historically, Open Source projects provide quite a lot of freedom for
development, build and test environment. That's fits individual contribution case well,
but degrades team performance and lowers its potential for synergy.

Therefore, Energi provides a simple and fast to startup development environment.
Its internals may change over time, but developer's interface should remain as consistent
as possible.

## Prerequisites

Development Environment is is based on [FutoIn CID](https://futoin.org/docs/cid/). Setup:

    pip install --user futoin-cid

*Note: please make sure pip is available first, see the [installation manual](https://futoin.org/docs/cid/install/)*

[Vagrant](https://www.vagrantup.com/intro/getting-started/install.html) builder VM
is supported as safe fallback and reference model.

Energi build system will gradually move towards CMake 3.2+, but all tools
and other dependencies should get automatically installed.

## Basics

Commands:

* `cid prepare` - install all missing dependencies and prepare working copy for building (run once).
* `cid build` - build the project (repeat on changes).
* `cid check` - run all available automatic tests (suited for CI).
* `cid package` - create setup packages.
* `cid run <entry-point> [<ep-args>]` - execute entry point located in build directory.

Paths relative to project root:

* `build/` - a **symlink** to current active build.
* `builds/{type}` - actual location of builds with specific configuration.
* `futoin.json` - a single place to configure FutoIn CID
* The rest is project source. All other build files may change or disappear over time.

## Advanced usage

**NOTE: this section may change over time: env vars may disappear, separate build type may
get merged into a single configuration, etc.**

Environment variables (direct usage is **DISCOURAGED**):

* `ENERGI_BUILD_DIR=builds/release` - actual location of out-of-source-tree binary artifacts.
* `MAKEJOBS=$(nproc)` - number of parallel GNU make jobs.

Extends commands (not part of standard prepare-build-check-package-promote flow):

* Preparation variations:
    * `cid run prepare-release` - same as with force environment config `cid prepare`.
    * `cid run prepare-debug` - prepare a Debug configuration.
    * `cid run prepare-coverage` - prepare a debug configuration with test coverage support.
* Testing details:
    * `cid run check-make [-- <make opts>]` - executes test available through legacy `make check`.
    * `cid run check-rpc [-- <test opts>]` - executes test available through `qa/pull-tester/rpc-tests.py`.
        - Example: `cid run check-rpc -- wallet.py`
* Execution:
    * `cid run energid [-- <args>]` - run last built `energid`.
    * `cid run energi-cli [-- <args>]` - run last built `energi-cli`.
    * `cid run energi-qt [-- <args>]` - run last built `energi-qt`.

## Optional Development VM

Vagrant on top of VirtualBox is selected as the most stable cross-platform
and the least intrusive solution which should fit any Linux, Windows or macos cases.

Current Ubuntu LTS is selected as reference operating system for the VM.

The working copy gets mounted under VM:/vagrant/ with bi-directional real-time synchronization
using VirtualBox Guest Additions (vboxfs mount).

Unlike common approach, the VM is headed (with GUI window). So, it should be possible to run
`energi-qt` and other GUI apps on demand.

This VM approach is preferred also for security reasons.

### Vagrant hints:

* `vagrant up` - get `builder` box running.
* `vagrant ssh builder` - SSH into the box.
* `cid ...` - run usual commands
