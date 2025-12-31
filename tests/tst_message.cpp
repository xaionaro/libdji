#include "tst_message.h"
#include <QDebug>
#include <QtEndian>
#include <QtTest>

using namespace dji;

void TestMessage::testSerialize() {

    Message msg;
    msg.subsystem = static_cast<SubsystemID>(0x0207);
    msg.msgId = static_cast<MessageID>(0x0400);
    msg.msgType = static_cast<MessageType>(0xC00746);
    msg.payload = QByteArray::fromHex("00");

    QByteArray serialized = msg.serialize();

    QCOMPARE(static_cast<uint8_t>(serialized[0]), 0x55);
    QCOMPARE(static_cast<uint8_t>(serialized[1]), 14);
    QCOMPARE(static_cast<uint8_t>(serialized[2]), 0x04);

    QCOMPARE(static_cast<uint8_t>(serialized[4]), 0x02);
    QCOMPARE(static_cast<uint8_t>(serialized[5]), 0x07);

    QCOMPARE(static_cast<uint8_t>(serialized[6]), 0x04);
    QCOMPARE(static_cast<uint8_t>(serialized[7]), 0x00);

    QCOMPARE(static_cast<uint8_t>(serialized[8]), 0xC0);
    QCOMPARE(static_cast<uint8_t>(serialized[9]), 0x07);
    QCOMPARE(static_cast<uint8_t>(serialized[10]), 0x46);

    QCOMPARE(static_cast<uint8_t>(serialized[11]), 0x00);

    uint8_t expectedHeaderCrc = dji::crc8(serialized.left(3));
    QCOMPARE(static_cast<uint8_t>(serialized[3]), expectedHeaderCrc);

    QByteArray fullNoCrc = serialized.left(serialized.size() - 2);
    uint16_t expectedFullCrc = dji::crc16(fullNoCrc);

    uint16_t actualFullCrc = qFromLittleEndian<uint16_t>(
        reinterpret_cast<const uchar *>(serialized.right(2).constData()));
    QCOMPARE(actualFullCrc, expectedFullCrc);
}

void TestMessage::testParse() {

    Message originalMsg;
    originalMsg.subsystem = static_cast<SubsystemID>(0x0207);
    originalMsg.msgId = static_cast<MessageID>(0x0400);
    originalMsg.msgType = static_cast<MessageType>(0xC00746);
    originalMsg.payload = QByteArray::fromHex("00");

    QByteArray validData = originalMsg.serialize();

    bool ok = false;
    Message msg = Message::parse(validData, &ok);

    QVERIFY(ok);
    QCOMPARE(msg.subsystem, originalMsg.subsystem);
    QCOMPARE(msg.msgId, originalMsg.msgId);
    QCOMPARE(msg.msgType, originalMsg.msgType);
    QCOMPARE(msg.payload, originalMsg.payload);
}

void TestMessage::testPackString() {
    QString s = "Hello";
    QByteArray packed = packString(s);

    QCOMPARE(packed.size(), 6);
    QCOMPARE(static_cast<uint8_t>(packed[0]), 5);
    QCOMPARE(packed.mid(1), QByteArray("Hello"));
}

void TestMessage::testPackURL() {
    QString s = "http://example.com";
    QByteArray packed = packURL(s);

    QCOMPARE(packed.size(), 2 + s.size());
    uint16_t len = qFromLittleEndian<uint16_t>(reinterpret_cast<const uchar *>(packed.constData()));
    QCOMPARE(len, s.size());
    QCOMPARE(packed.mid(2), s.toUtf8());
}
