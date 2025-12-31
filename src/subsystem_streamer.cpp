#include "dji/subsystem_streamer.h"
#include "dji/constants.h"
#include "dji/device.h"
#include <QDebug>
#include <QtEndian>

namespace dji {

SubsystemStreamer::SubsystemStreamer(Device *device) : m_device(device) {
}

void SubsystemStreamer::prepareToLiveStream() {
    emit log("Preparing to live stream (Stage 1)...");
    m_state = State::PreparingStage1;

    sendMessagePrepareToLiveStreamStage1();
}

void SubsystemStreamer::startLiveStream(Resolution resolution, uint16_t bitrateKbps, FPS fps,
                                        const QString &rtmpURL) {
    emit log(QString("Starting live stream to %1").arg(rtmpURL));
    m_pendingResolution = resolution;
    m_pendingBitrate = bitrateKbps;
    m_pendingFps = fps;
    m_pendingRtmpUrl = rtmpURL;

    sendMessageConfigureLiveStream(resolution, bitrateKbps, fps, rtmpURL);

    m_state = State::Starting;
    sendMessageStartLiveStream();
}

void SubsystemStreamer::stopLiveStream() {
    emit log("Stopping live stream...");
    m_state = State::Stopping;
    sendMessageStopLiveStream();
}

void SubsystemStreamer::handleMessage(const Message &msg) {
    if (msg.msgType == MessageType::PrepareToLiveStreamResult) {
        if (m_state == State::PreparingStage1) {

            if (msg.payload.size() == 1 && static_cast<uint8_t>(msg.payload[0]) == 0x00) {
                emit log("PrepareToLiveStream Stage 1 success. Sending Stage 2...");
                m_state = State::PreparingStage2;
                sendMessagePrepareToLiveStreamStage2();
            } else {
                emit error(QString("PrepareToLiveStream Stage 1 failed. Payload: %1")
                               .arg(QString(msg.payload.toHex())));
            }
        }
    } else if (msg.msgType == MessageType::StartStopStreamingResult) {

        if (m_state == State::PreparingStage2) {
            emit log("PrepareToLiveStream Stage 2 success.");
            m_state = State::Idle;
            emit prepareToLiveStreamComplete();
        } else if (m_state == State::Starting) {

            if (msg.msgId == MessageID::StartStreaming) {
                emit log("StartLiveStream success.");
                m_state = State::Idle;
                emit startLiveStreamComplete();
            }
        } else if (m_state == State::Stopping) {

            emit log("StopLiveStream success.");
            m_state = State::Idle;
            emit stopLiveStreamComplete();
        }
    } else if (msg.msgType == MessageType::StreamingStatus) {
        if (msg.payload.size() >= 21) {
            int battery = static_cast<uint8_t>(msg.payload[20]);
            emit batteryPercentageChanged(battery);
        }
    }
}

void SubsystemStreamer::sendMessagePrepareToLiveStreamStage1() {
    Message msg;
    msg.subsystem = subsystemID();
    msg.msgId = MessageID::PrepareToLiveStreamStage1;
    msg.msgType = MessageType::PrepareToLiveStream;
    msg.payload = QByteArray::fromHex("1A");
    m_device->sendMessage(msg, true);
}

void SubsystemStreamer::sendMessagePrepareToLiveStreamStage2() {
    Message msg;
    msg.subsystem = subsystemID();
    msg.msgId = MessageID::StartStreaming;

    msg.msgId = MessageID::StartStreaming;
    msg.msgType = MessageType::StartStopStreaming;
    msg.payload = QByteArray::fromHex("00011C00");
    m_device->sendMessage(msg, true);
}

void SubsystemStreamer::sendMessageConfigureLiveStream(Resolution resolution, uint16_t bitrateKbps,
                                                       FPS fps, const QString &rtmpURL) {

    QByteArray payload;
    payload.append('\0');
    payload.append(static_cast<char>(deviceTypeToByte(m_device->deviceType())));
    payload.append('\0');
    payload.append(static_cast<char>(resolution));

    uint16_t br = qToLittleEndian(bitrateKbps);
    payload.append(reinterpret_cast<const char *>(&br), 2);

    payload.append(QByteArray::fromHex("0200"));
    payload.append(static_cast<char>(fps));
    payload.append(QByteArray::fromHex("000000"));
    payload.append(packURL(rtmpURL));

    Message msg;
    msg.subsystem = subsystemID();
    msg.msgId = MessageID::ConfigureStreaming;
    msg.msgType = MessageType::ConfigureStreaming;
    msg.payload = payload;

    m_device->sendMessage(msg, true);
}

void SubsystemStreamer::sendMessageStartLiveStream() {

    Message msg;
    msg.subsystem = subsystemID();
    msg.msgId = MessageID::StartStreaming;
    msg.msgType = MessageType::StartStopStreaming;
    msg.payload = QByteArray::fromHex("01011A000101");

    m_device->sendMessage(msg, true);
}

void SubsystemStreamer::sendMessageStopLiveStream() {

    Message msg;
    msg.subsystem = subsystemID();
    msg.msgId = MessageID::StopStreaming;
    msg.msgType = MessageType::StartStopStreaming;
    msg.payload = QByteArray::fromHex("01011A000102");

    m_device->sendMessage(msg, true);
}

} // namespace dji
