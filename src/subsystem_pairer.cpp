#include "dji/subsystem_pairer.h"
#include "dji/device.h"
#include <QDebug>
#include <QTimer>

namespace dji {

static const QString defaultPINCode = "5160";

SubsystemPairer::SubsystemPairer(Device *device) : m_device(device) {
}

void SubsystemPairer::pair() {
    if (m_state != State::Idle)
        return;

    emit log("Starting pairing process...");
    m_state = State::WaitingForStatus;

    sendRequestStartPairing();

    sendMessageSetPairingPIN(defaultPINCode);
}

void SubsystemPairer::connectToWiFi(const QString &ssid, const QString &psk) {
    emit log(QString("Connecting to WiFi SSID: %1").arg(ssid));

    sendMessageConnectToWiFi(ssid, psk);
}

void SubsystemPairer::startScanningWiFi() {
    emit log("Starting WiFi scan...");
    sendMessageStartScanningWiFi();
}

void SubsystemPairer::handleMessage(const Message &msg) {
    if (msg.msgType == MessageType::PairingStatus) {

        if (msg.payload.size() >= 2 && static_cast<uint8_t>(msg.payload[1]) == 0x01) {
            emit log("Device is already paired.");
            m_state = State::Idle;
            emit pairingComplete();
        }
    } else if (msg.msgType == MessageType::PairingPINApproved) {
        emit log("PIN approved. Finalizing pairing...");

        sendMessagePairingStage1();
        sendMessagePairingStage2();

        m_state = State::Idle;
        emit pairingComplete();
    } else if (msg.msgType == MessageType::ConnectToWiFiResult) {
        if (msg.payload.size() >= 2 && msg.payload[0] == 0x00 && msg.payload[1] == 0x00) {
            emit log("WiFi connected successfully.");
            emit wifiConnected();
        } else {
            QString err =
                QString("WiFi connection failed. Payload: %1").arg(QString(msg.payload.toHex()));
            emit error(err);
        }
    } else if (msg.msgType == MessageType::WiFiScanReport) {
        emit log("Received WiFi scan report.");
        emit wifiScanReport(msg.payload);
    }
}

void SubsystemPairer::sendRequestStartPairing() {

    m_device->sendRawPairing(QByteArray::fromHex("0100"));
}

void SubsystemPairer::sendMessageSetPairingPIN(const QString &pinCode) {

    QByteArray payload;
    payload.append(packString("001749319286102"));
    payload.append(packString(pinCode));

    Message msg;
    msg.subsystem = subsystemID();
    msg.msgId = MessageID::SetPairingPIN;
    msg.msgType = MessageType::SetPairingPIN;
    msg.payload = payload;

    m_device->sendMessage(msg, true);
}

void SubsystemPairer::sendMessagePairingStage1() {

    Message msg;
    msg.subsystem = subsystemID();
    msg.msgId = MessageID::PairingStarted;

    msg.msgId = MessageID::PairingStage1;
    msg.msgType = MessageType::PairingStage1;
    msg.payload = QByteArray::fromHex("00");

    m_device->sendMessage(msg, true);
}

void SubsystemPairer::sendMessagePairingStage2() {

    Message msg;
    msg.subsystem = SubsystemID::OneMorePairer;
    msg.msgId = MessageID::PairingStage2;
    msg.msgType = MessageType::PairingStage2;
    msg.payload = QByteArray::fromHex("3131000000");

    m_device->sendMessage(msg, true);
}

void SubsystemPairer::sendMessageStartScanningWiFi() {
    Message msg;
    msg.subsystem = subsystemID();
    msg.msgId = MessageID::StartScanningWiFi;
    msg.msgType = MessageType::StartScanningWiFi;

    m_device->sendMessage(msg, true);
}

void SubsystemPairer::sendMessageConnectToWiFi(const QString &ssid, const QString &psk) {
    QByteArray payload;
    payload.append(packString(ssid));
    payload.append(packString(psk));

    Message msg;
    msg.subsystem = subsystemID();
    msg.msgId = MessageID::ConnectToWiFi;
    msg.msgType = MessageType::ConnectToWiFi;
    msg.payload = payload;

    m_device->sendMessage(msg, true);
}

} // namespace dji
