/**
 * @file tst_main.cpp
 * @brief Main entry point for libdji unit tests.
 */

#include <QCoreApplication>
#include <QTest>

#include "tst_connect_flow.h"
#include "tst_crc.h"
#include "tst_message.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    int status = 0;

    {
        TestCRC tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    {
        TestMessage tm;
        status |= QTest::qExec(&tm, argc, argv);
    }

    {
        TestConnectWifiAndStreaming tcf;
        status |= QTest::qExec(&tcf, argc, argv);
    }

    return status;
}
