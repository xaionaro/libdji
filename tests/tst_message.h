#pragma once

#include "dji/message.h"
#include <QObject>
#include <QTest>

class TestMessage : public QObject {
    Q_OBJECT
private slots:
    void testSerialize();
    void testParse();
    void testPackString();
    void testPackURL();
};
