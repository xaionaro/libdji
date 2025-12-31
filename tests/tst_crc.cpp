#include "tst_crc.h"
#include "dji/crc.h"
#include <QByteArray>
#include <QtTest>

void TestCRC::testCRC8() {
    QByteArray data("123456789");
    uint8_t result = dji::crc8(data);

    QCOMPARE(result, 0xFB);
}

void TestCRC::testCRC16() {
    QByteArray data("123456789");
    uint16_t result = dji::crc16(data);

    QCOMPARE(result, 0x7109);
}
