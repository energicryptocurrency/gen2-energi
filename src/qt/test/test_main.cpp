// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2017 The Energi Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/energi-config.h"
#endif

#include "util.h"
#include "uritests.h"
#include "compattests.h"
#include "trafficgraphdatatests.h"

#ifdef ENABLE_WALLET
#include "paymentservertests.h"
#endif

#include <QCoreApplication>
#include <QObject>
#include <QTest>

#include <openssl/ssl.h>

#if defined(QT_STATICPLUGIN) && QT_VERSION < 0x050000
#include <QtPlugin>
Q_IMPORT_PLUGIN(qcncodecs)
Q_IMPORT_PLUGIN(qjpcodecs)
Q_IMPORT_PLUGIN(qtwcodecs)
Q_IMPORT_PLUGIN(qkrcodecs)
#endif

// append the test case name to the output file name to allow running multiple test suites without overwriting the output file
QStringList getArguments(QStringList const & app_args, QString objectName)
{
    QStringList result(app_args);
    result.replaceInStrings(QRegExp("^(.*)\\.(.+)$"), QString("\\1_") + objectName + QString(".\\2"));
    return result;
}

// This is all you need to run all the tests
int main(int argc, char *argv[])
{
    SetupEnvironment();
    bool fInvalid = false;

    // Don't remove this, it's needed to access
    // QCoreApplication:: in the tests
    QCoreApplication app(argc, argv);
    app.setApplicationName("Energi-Qt-test");

    SSL_library_init();

    auto const & app_args = app.arguments();

    URITests test1;
    test1.setObjectName("URITests");
    if (QTest::qExec(&test1, getArguments(app_args, test1.objectName())) != 0)
        fInvalid = true;
#ifdef ENABLE_WALLET
    PaymentServerTests test2;
    test2.setObjectName("PaymentServerTests");
    if (QTest::qExec(&test2, getArguments(app_args, test2.objectName())) != 0)
        fInvalid = true;
#endif
    CompatTests test4;
    test4.setObjectName("CompatTests");
    if (QTest::qExec(&test4, getArguments(app_args, test4.objectName())) != 0)
        fInvalid = true;

    TrafficGraphDataTests test5;
    test5.setObjectName("TrafficGraphDataTests");
    if (QTest::qExec(&test5, getArguments(app_args, test5.objectName())) != 0)
        fInvalid = true;

    return fInvalid;
}
