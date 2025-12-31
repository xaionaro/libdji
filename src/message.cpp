#include "dji/message.h"
#include "dji/crc.h"
#include <QDebug>
#include <QtEndian>

namespace dji {

QByteArray packString(const QString &s) {
    QByteArray b = s.toUtf8();
    if (b.size() > 255) {
        qWarning() << "String too long for packString:" << s;
    }
    uint8_t len = static_cast<uint8_t>(b.size());
    QByteArray res;
    res.append(static_cast<char>(len));
    res.append(b);
    return res;
}

QByteArray packURL(const QString &s) {
    QByteArray b = s.toUtf8();
    if (b.size() > 65535) {
        qWarning() << "String too long for packURL:" << s;
    }
    uint16_t len = qToLittleEndian(static_cast<uint16_t>(b.size()));
    QByteArray res;
    res.append(reinterpret_cast<const char *>(&len), 2);
    res.append(b);
    return res;
}

QByteArray Message::serialize() const {
    if (payload.size() > (255 - 13)) {
        qCritical() << "Payload too long:" << payload.size();
    }

    QByteArray buf;
    buf.append(static_cast<char>(0x55));
    buf.append(static_cast<char>(13 + payload.size()));
    buf.append(static_cast<char>(0x04));

    buf.append(static_cast<char>(crc8(buf)));

    uint16_t subBE = qToBigEndian(static_cast<uint16_t>(subsystem));
    buf.append(reinterpret_cast<const char *>(&subBE), 2);

    uint16_t idBE = qToBigEndian(static_cast<uint16_t>(msgId));
    buf.append(reinterpret_cast<const char *>(&idBE), 2);

    uint32_t typeBE = qToBigEndian(static_cast<uint32_t>(msgType));

    const char *typePtr = reinterpret_cast<const char *>(&typeBE);
    buf.append(typePtr + 1, 3);

    buf.append(payload);

    uint16_t fullCrc = crc16(buf);

    uint16_t fullCrcLE = qToLittleEndian(fullCrc);
    buf.append(reinterpret_cast<const char *>(&fullCrcLE), 2);

    return buf;
}

Message Message::parse(const QByteArray &data, bool *ok) {
    if (ok)
        *ok = false;
    if (data.size() < 13) {
        qWarning() << "Message too short:" << data.size();
        return {};
    }

    if (static_cast<uint8_t>(data[0]) != 0x55) {
        qWarning() << "Invalid magic:" << static_cast<uint8_t>(data[0]);
        return {};
    }

    uint8_t length = static_cast<uint8_t>(data[1]);
    if (length != data.size()) {

        if (static_cast<uint64_t>(length) > static_cast<uint64_t>(data.size())) {
            qWarning() << "Not enough data for length:" << length;
            return {};
        }
    }

    uint8_t version = static_cast<uint8_t>(data[2]);
    if (version != 0x04) {
        qWarning() << "Invalid version:" << version;
        return {};
    }

    uint8_t headerCRC = static_cast<uint8_t>(data[3]);
    QByteArray header = data.left(3);
    if (crc8(header) != headerCRC) {
        qWarning() << "Header CRC mismatch";
        return {};
    }

    QByteArray msgWithoutCRC = data.left(length - 2);
    QByteArray providedCRCBytes = data.mid(length - 2, 2);
    uint16_t providedCRC =
        qFromLittleEndian<uint16_t>(reinterpret_cast<const uchar *>(providedCRCBytes.data()));

    if (crc16(msgWithoutCRC) != providedCRC) {
        qWarning() << "Full CRC mismatch";
        return {};
    }

    uint16_t subsystem =
        qFromBigEndian<uint16_t>(reinterpret_cast<const uchar *>(data.mid(4, 2).data()));
    uint16_t msgId =
        qFromBigEndian<uint16_t>(reinterpret_cast<const uchar *>(data.mid(6, 2).data()));

    uint32_t msgType = 0;

    msgType = (static_cast<uint8_t>(data[8]) << 16) | (static_cast<uint8_t>(data[9]) << 8) |
              static_cast<uint8_t>(data[10]);

    QByteArray payload = data.mid(11, length - 13);

    if (ok)
        *ok = true;
    return {static_cast<SubsystemID>(subsystem), static_cast<MessageID>(msgId),
            static_cast<MessageType>(msgType), payload};
}

} // namespace dji
