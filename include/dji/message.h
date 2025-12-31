#ifndef DJI_MESSAGE_H
#define DJI_MESSAGE_H

#include "dji/constants.h"
#include <QByteArray>
#include <QString>
#include <cstdint>

namespace dji {

struct Message {
    SubsystemID subsystem = static_cast<SubsystemID>(0);
    MessageID msgId = static_cast<MessageID>(0);
    MessageType msgType = static_cast<MessageType>(0);
    QByteArray payload;

    QByteArray serialize() const;
    static Message parse(const QByteArray &data, bool *ok = nullptr);
};

uint8_t crc8(const QByteArray &data);
uint16_t crc16(const QByteArray &data);

QByteArray packString(const QString &s);
QByteArray packURL(const QString &s);

} // namespace dji

#endif
