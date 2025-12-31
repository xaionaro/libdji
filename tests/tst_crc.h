#pragma once

#include "dji/crc.h"
#include <QObject>
#include <QTest>

class TestCRC : public QObject {
    Q_OBJECT
private slots:
    void testCRC8();
    void testCRC16();
};
