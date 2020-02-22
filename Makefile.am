# Copyright (c) 2013-2016 The Bitcoin Core developers
# Copyright (c) 2014-2018 The Dash Core developers
# Copyright (c) 2017-2019 The Energi Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

ACLOCAL_AMFLAGS = -I build-aux/m4
SUBDIRS = src
if ENABLE_MAN
SUBDIRS += doc/man
endif
.PHONY: deploy FORCE

GZIP_ENV="-9n"
export PYTHONPATH

if BUILD_BITCOIN_LIBS
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libenergiconsensus.pc
endif

BITCOIND_BIN=$(top_builddir)/src/$(BITCOIN_DAEMON_NAME)$(EXEEXT)
BITCOIN_QT_BIN=$(top_builddir)/src/qt/$(BITCOIN_GUI_NAME)$(EXEEXT)
BITCOIN_CLI_BIN=$(top_builddir)/src/$(BITCOIN_CLI_NAME)$(EXEEXT)

if TARGET_DARWIN
ENERGI_DEBUG_NAME=$(PACKAGE)-$(PACKAGE_VERSION)-macos-debug
endif
if TARGET_WINDOWS
ENERGI_DEBUG_NAME=$(PACKAGE)-$(PACKAGE_VERSION)-win$(WINDOWS_BITS)-debug
endif
if TARGET_LINUX
ENERGI_DEBUG_NAME=$(PACKAGE)-$(PACKAGE_VERSION)-linux-debug
endif
ENERGI_DEBUG_TAR=$(ENERGI_DEBUG_NAME).tgz

ENERGI_LINUX_TAR=$(PACKAGE)-$(PACKAGE_VERSION)-linux.tar.gz
BITCOIN_WIN_INSTALLER=$(PACKAGE)-$(PACKAGE_VERSION)-win$(WINDOWS_BITS)-setup$(EXEEXT)
ENERGI_OSX_DMG=$(PACKAGE)-$(PACKAGE_VERSION)-macos.dmg

empty :=
space := $(empty) $(empty)

OSX_APP=Energi-Qt.app
OSX_VOLNAME = $(subst $(space),-,$(PACKAGE_NAME))
OSX_BACKGROUND_IMAGE=background.tiff
#OSX_BACKGROUND_IMAGE_DPIS=36 72
#OSX_DSSTORE_GEN=$(top_srcdir)/contrib/macdeploy/custom_dsstore.py
OSX_DEPLOY_SCRIPT=$(top_srcdir)/contrib/macdeploy/macdeployqtplus
OSX_FANCY_PLIST=$(top_srcdir)/contrib/macdeploy/fancy.plist
OSX_INSTALLER_ICONS=$(top_srcdir)/src/qt/res/icons/energi.icns
OSX_PLIST=$(top_builddir)/share/qt/Info.plist #not installed
OSX_QT_TRANSLATIONS = da,de,es,hu,ru,uk,zh_CN,zh_TW

DIST_DOCS = $(wildcard doc/*.md) $(wildcard doc/release-notes/*.md)
DIST_CONTRIB = $(top_srcdir)/contrib/dash-cli.bash-completion \
	       $(top_srcdir)/contrib/dash-tx.bash-completion \
	       $(top_srcdir)/contrib/dashd.bash-completion \
	       $(top_srcdir)/contrib/init

BIN_CHECKS=$(top_srcdir)/contrib/devtools/symbol-check.py \
           $(top_srcdir)/contrib/devtools/security-check.py

WINDOWS_PACKAGING = $(top_srcdir)/share/pixmaps/energi.ico \
  $(top_srcdir)/share/pixmaps/nsis-header.bmp \
  $(top_srcdir)/share/pixmaps/nsis-wizard.bmp \
  $(top_srcdir)/doc/README_windows.txt

OSX_PACKAGING = $(OSX_DEPLOY_SCRIPT) $(OSX_FANCY_PLIST) $(OSX_INSTALLER_ICONS) $(OSX_BASE_LPROJ_DIR) \
  $(top_srcdir)/contrib/macdeploy/$(OSX_BACKGROUND_IMAGE) \
  $(top_srcdir)/contrib/macdeploy/DS_Store \
  $(top_srcdir)/contrib/macdeploy/detached-sig-apply.sh \
  $(top_srcdir)/contrib/macdeploy/detached-sig-create.sh

COVERAGE_INFO = baseline_filtered_combined.info baseline.info \
  leveldb_baseline.info test_energi_filtered.info total_coverage.info \
  baseline_filtered.info rpc_test.info rpc_test_filtered.info \
  leveldb_baseline_filtered.info test_energi_coverage.info test_energi.info

dist-hook:
	-$(GIT) archive --format=tar HEAD -- src/clientversion.cpp | $(AMTAR) -C $(top_distdir) -xf -

split-debug: all-recursive
	$(MKDIR_P) $(ENERGI_DEBUG_NAME)
if TARGET_DARWIN
	$(INSTALL) $(BITCOIND_BIN) $(top_builddir)/$(ENERGI_DEBUG_NAME)/energid-debug
	$(INSTALL) $(BITCOIN_QT_BIN) $(top_builddir)/$(ENERGI_DEBUG_NAME)/energi-qt-debug
	$(INSTALL) $(BITCOIN_CLI_BIN) $(top_builddir)/$(ENERGI_DEBUG_NAME)/energid-cli-debug
else
	$(OBJCOPY) --enable-deterministic-archives -p --only-keep-debug $(BITCOIND_BIN) $(top_builddir)/$(ENERGI_DEBUG_NAME)/energid.dbg
	$(OBJCOPY) --enable-deterministic-archives -p --only-keep-debug $(BITCOIN_QT_BIN) $(top_builddir)/$(ENERGI_DEBUG_NAME)/energi-qt.dbg
	$(OBJCOPY) --enable-deterministic-archives -p --only-keep-debug $(BITCOIN_CLI_BIN) $(top_builddir)/$(ENERGI_DEBUG_NAME)/energi-cli.dbg
endif
	tar czf $(ENERGI_DEBUG_TAR) -C $(top_builddir) $(ENERGI_DEBUG_NAME)

$(ENERGI_LINUX_TAR): split-debug
	$(MKDIR_P) $(top_builddir)/$(PACKAGE)-$(PACKAGE_VERSION)/bin
	STRIPPROG="$(STRIP)" $(INSTALL_STRIP_PROGRAM) $(BITCOIND_BIN) $(top_builddir)/$(PACKAGE)-$(PACKAGE_VERSION)/bin
	STRIPPROG="$(STRIP)" $(INSTALL_STRIP_PROGRAM) $(BITCOIN_QT_BIN) $(top_builddir)/$(PACKAGE)-$(PACKAGE_VERSION)/bin
	STRIPPROG="$(STRIP)" $(INSTALL_STRIP_PROGRAM) $(BITCOIN_CLI_BIN) $(top_builddir)/$(PACKAGE)-$(PACKAGE_VERSION)/bin
	$(OBJCOPY) --enable-deterministic-archives -p --add-gnu-debuglink=$(ENERGI_DEBUG_NAME)/energid.dbg $(top_builddir)/$(PACKAGE)-$(PACKAGE_VERSION)/bin/energid
	$(OBJCOPY) --enable-deterministic-archives -p --add-gnu-debuglink=$(ENERGI_DEBUG_NAME)/energi-qt.dbg $(top_builddir)/$(PACKAGE)-$(PACKAGE_VERSION)/bin/energi-qt
	$(OBJCOPY) --enable-deterministic-archives -p --add-gnu-debuglink=$(ENERGI_DEBUG_NAME)/energi-cli.dbg $(top_builddir)/$(PACKAGE)-$(PACKAGE_VERSION)/bin/energi-cli
	$(top_builddir)/contrib/devtools/mini-debug.sh $(BITCOIND_BIN) $(top_builddir)/$(PACKAGE)-$(PACKAGE_VERSION)/bin/energid
	$(top_builddir)/contrib/devtools/mini-debug.sh $(BITCOIN_QT_BIN) $(top_builddir)/$(PACKAGE)-$(PACKAGE_VERSION)/bin/energi-qt
	$(top_builddir)/contrib/devtools/mini-debug.sh $(BITCOIN_CLI_BIN) $(top_builddir)/$(PACKAGE)-$(PACKAGE_VERSION)/bin/energi-cli
	tar czf $(ENERGI_LINUX_TAR) -C $(top_builddir) $(PACKAGE)-$(PACKAGE_VERSION)

$(BITCOIN_WIN_INSTALLER): split-debug
	$(MKDIR_P) $(top_builddir)/release
	STRIPPROG="$(STRIP)" $(INSTALL_STRIP_PROGRAM) $(BITCOIND_BIN) $(top_builddir)/release
	STRIPPROG="$(STRIP)" $(INSTALL_STRIP_PROGRAM) $(BITCOIN_QT_BIN) $(top_builddir)/release
	STRIPPROG="$(STRIP)" $(INSTALL_STRIP_PROGRAM) $(BITCOIN_CLI_BIN) $(top_builddir)/release
	$(OBJCOPY) --enable-deterministic-archives -p --add-gnu-debuglink=$(ENERGI_DEBUG_NAME)/energid.dbg $(top_builddir)/release/energid$(EXEEXT)
	$(OBJCOPY) --enable-deterministic-archives -p --add-gnu-debuglink=$(ENERGI_DEBUG_NAME)/energi-qt.dbg $(top_builddir)/release/energi-qt$(EXEEXT)
	$(OBJCOPY) --enable-deterministic-archives -p --add-gnu-debuglink=$(ENERGI_DEBUG_NAME)/energi-cli.dbg $(top_builddir)/release/energi-cli$(EXEEXT)
	@test -f $(MAKENSIS) && $(MAKENSIS) -V2 $(top_builddir)/share/setup.nsi || \
	  echo error: could not build $@
	@echo built $@

$(OSX_APP)/Contents/PkgInfo:
	$(MKDIR_P) $(@D)
	@echo "APPL????" > $@

$(OSX_APP)/Contents/Resources/empty.lproj:
	$(MKDIR_P) $(@D)
	@touch $@

$(OSX_APP)/Contents/Info.plist: $(OSX_PLIST)
	$(MKDIR_P) $(@D)
	$(INSTALL_DATA) $< $@

$(OSX_APP)/Contents/Resources/energi.icns: $(OSX_INSTALLER_ICONS)
	$(MKDIR_P) $(@D)
	$(INSTALL_DATA) $< $@

$(OSX_APP)/Contents/MacOS/Energi-Qt: $(BITCOIN_QT_BIN)
	$(MKDIR_P) $(@D)
	STRIPPROG="$(STRIP)" $(INSTALL_STRIP_PROGRAM)  $< $@

$(OSX_APP)/Contents/Resources/Base.lproj/InfoPlist.strings:
	$(MKDIR_P) $(@D)
	echo '{	CFBundleDisplayName = "$(PACKAGE_NAME)"; CFBundleName = "$(PACKAGE_NAME)"; }' > $@

OSX_APP_BUILT=$(OSX_APP)/Contents/PkgInfo $(OSX_APP)/Contents/Resources/empty.lproj \
  $(OSX_APP)/Contents/Resources/energi.icns $(OSX_APP)/Contents/Info.plist \
  $(OSX_APP)/Contents/MacOS/Energi-Qt $(OSX_APP)/Contents/Resources/Base.lproj/InfoPlist.strings

osx_volname:
	echo $(OSX_VOLNAME) >$@

if BUILD_DARWIN
$(ENERGI_OSX_DMG): $(OSX_APP_BUILT) $(OSX_PACKAGING)
	$(PYTHON) $(OSX_DEPLOY_SCRIPT) $(OSX_APP) -add-qt-tr $(OSX_QT_TRANSLATIONS) -translations-dir=$(QT_TRANSLATION_DIR) -dmg $(ENERGI_OSX_DMG) -fancy $(OSX_FANCY_PLIST) -verbose 2 -volname $(OSX_VOLNAME)

deploydir: $(ENERGI_OSX_DMG)
else
APP_DIST_DIR=$(top_builddir)/dist
APP_DIST_EXTRAS=$(APP_DIST_DIR)/.background/$(OSX_BACKGROUND_IMAGE) $(APP_DIST_DIR)/.DS_Store $(APP_DIST_DIR)/Applications

$(APP_DIST_DIR)/Applications:
	@rm -f $@
	@cd $(@D); $(LN_S) /Applications $(@F)

$(APP_DIST_EXTRAS): $(APP_DIST_DIR)/$(OSX_APP)/Contents/MacOS/Energi-Qt

$(ENERGI_OSX_DMG): $(APP_DIST_EXTRAS)
	$(GENISOIMAGE) -no-cache-inodes -D -l -probe -V "$(OSX_VOLNAME)" -no-pad -r -dir-mode 0755 -apple -o $@ dist

$(APP_DIST_DIR)/.background/$(OSX_BACKGROUND_IMAGE): contrib/macdeploy/$(OSX_BACKGROUND_IMAGE)
	$(MKDIR_P) $(@D)
	$(INSTALL) $< $@
$(APP_DIST_DIR)/.DS_Store: contrib/macdeploy/DS_Store
	$(INSTALL) $< $@

$(APP_DIST_DIR)/$(OSX_APP)/Contents/MacOS/Energi-Qt: $(OSX_APP_BUILT) $(OSX_PACKAGING)
	INSTALLNAMETOOL=$(INSTALLNAMETOOL)  OTOOL=$(OTOOL) STRIP=$(STRIP) $(PYTHON) $(OSX_DEPLOY_SCRIPT) $(OSX_APP) -translations-dir=$(QT_TRANSLATION_DIR) -add-qt-tr $(OSX_QT_TRANSLATIONS) -verbose 2

deploydir: $(APP_DIST_EXTRAS)
endif

if TARGET_LINUX
deploy: $(ENERGI_LINUX_TAR)
endif
if TARGET_DARWIN
appbundle: $(OSX_APP_BUILT)
deploy: $(ENERGI_OSX_DMG) split-debug
endif
if TARGET_WINDOWS
deploy: $(BITCOIN_WIN_INSTALLER)
endif

$(BITCOIN_QT_BIN): FORCE
	$(MAKE) -C src qt/$(@F)

$(BITCOIND_BIN): FORCE
	$(MAKE) -C src $(@F)

$(BITCOIN_CLI_BIN): FORCE
	$(MAKE) -C src $(@F)

if USE_LCOV

baseline.info:
	$(LCOV) -c -i -d $(abs_builddir)/src -o $@

baseline_filtered.info: baseline.info
	$(LCOV) -r $< "/usr/include/*" -o $@

leveldb_baseline.info: baseline_filtered.info
	$(LCOV) -c -i -d $(abs_builddir)/src/leveldb -b $(abs_builddir)/src/leveldb -o $@

leveldb_baseline_filtered.info: leveldb_baseline.info
	$(LCOV) -r $< "/usr/include/*" -o $@

baseline_filtered_combined.info: leveldb_baseline_filtered.info baseline_filtered.info
	$(LCOV) -a leveldb_baseline_filtered.info -a baseline_filtered.info -o $@

test_energi.info: baseline_filtered_combined.info
	$(MAKE) -C src/ check
	$(LCOV) -c -d $(abs_builddir)/src -t test_energi -o $@
	$(LCOV) -z -d $(abs_builddir)/src
	$(LCOV) -z -d $(abs_builddir)/src/leveldb

test_energi_filtered.info: test_energi.info
	$(LCOV) -r $< "/usr/include/*" -o $@

rpc_test.info: test_energi_filtered.info
	-@TIMEOUT=15 python qa/pull-tester/rpc-tests.py $(EXTENDED_RPC_TESTS)
	$(LCOV) -c -d $(abs_builddir)/src --t rpc-tests -o $@
	$(LCOV) -z -d $(abs_builddir)/src
	$(LCOV) -z -d $(abs_builddir)/src/leveldb

rpc_test_filtered.info: rpc_test.info
	$(LCOV) -r $< "/usr/include/*" -o $@

test_energi_coverage.info: baseline_filtered_combined.info test_energi_filtered.info
	$(LCOV) -a baseline_filtered.info -a leveldb_baseline_filtered.info -a test_energi_filtered.info -o $@

total_coverage.info: baseline_filtered_combined.info test_energi_filtered.info rpc_test_filtered.info
	$(LCOV) -a baseline_filtered.info -a leveldb_baseline_filtered.info -a test_energi_filtered.info -a rpc_test_filtered.info -o $@ | $(GREP) "\%" | $(AWK) '{ print substr($$3,2,50) "/" $$5 }' > coverage_percent.txt

test_energi.coverage/.dirstamp:  test_energi_coverage.info
	$(GENHTML) -s $< -o $(@D)
	@touch $@

total.coverage/.dirstamp: total_coverage.info
	$(GENHTML) -s $< -o $(@D)
	@touch $@

cov: test_energi.coverage/.dirstamp total.coverage/.dirstamp

endif

dist_noinst_SCRIPTS = autogen.sh

EXTRA_DIST = $(top_srcdir)/share/genbuild.sh qa/pull-tester/rpc-tests.py qa/rpc-tests $(DIST_CONTRIB) $(DIST_DOCS) $(WINDOWS_PACKAGING) $(OSX_PACKAGING) $(BIN_CHECKS)

CLEANFILES = $(ENERGI_LINUX_TAR) $(ENERGI_OSX_DMG) $(BITCOIN_WIN_INSTALLER) $(ENERGI_DEBUG_TAR)

# This file is problematic for out-of-tree builds if it exists.
DISTCLEANFILES = qa/pull-tester/tests_config.pyc

.INTERMEDIATE: $(COVERAGE_INFO)

DISTCHECK_CONFIGURE_FLAGS = --enable-man

clean-local:
	rm -rf coverage_percent.txt test_energi.coverage/ total.coverage/ qa/tmp/ cache/ $(OSX_APP)
	rm -rf qa/pull-tester/__pycache__
