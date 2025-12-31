#include "dji/subsystem_configurer.h"
#include "dji/device.h"
#include <QDebug>

namespace dji {

SubsystemConfigurer::SubsystemConfigurer(Device *device) : m_device(device) {
}

void SubsystemConfigurer::setImageStabilization(ImageStabilization v) {
    emit log("[DJI-BLE] " + QString("Setting image stabilization to %1").arg(static_cast<int>(v)));
    sendMessageSetImageStabilization(v);
}

void SubsystemConfigurer::handleMessage(const Message &msg) {
    if (msg.subsystem == SubsystemID::Configurer && msg.msgType == MessageType::Configure) {
        emit log("[DJI-BLE] " +
                 QString("Received configurer result: %1").arg(QString(msg.payload.toHex())));
    }
}

void SubsystemConfigurer::sendMessageSetImageStabilization(ImageStabilization v) {

    QByteArray payload;
    payload.append(QByteArray::fromHex("0101"));
    payload.append(static_cast<char>(deviceTypeToStabilizationByte(m_device->deviceType())));
    payload.append(QByteArray::fromHex("0001"));
    payload.append(static_cast<char>(v));

    Message msg;
    msg.subsystem = subsystemID();
    msg.msgId = static_cast<MessageID>(0);
    msg.msgType = MessageType::Configure;
    msg.payload = payload;

    m_device->sendMessage(msg, true);
}

} // namespace dji
