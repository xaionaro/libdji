/**
 * @file tst_connect_flow.cpp
 * @brief Unit tests for the DJI connection flow using a MockDevice.
 */

#include "tst_connect_flow.h"
#include "dji/device.h"
#include "dji/device_manager.h"
#include "dji/message.h"
#include "dji/subsystem_configurer.h"
#include "dji/subsystem_pairer.h"
#include "dji/subsystem_streamer.h"
#include <QObject>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <functional>

class MockDevice : public dji::Device {
    Q_OBJECT
public:
    explicit MockDevice(QObject *parent = nullptr)
        : dji::Device(QBluetoothDeviceInfo(), dji::DeviceType::OsmoPocket3, parent) {
    }

    void sendMessage(const dji::Message &msg, bool noResponse = true) override {
        Q_UNUSED(noResponse);

        qDebug().noquote() << "SENT_HEX:" << msg.serialize().toHex().toUpper();
        emit messageSent(msg);

        // Always respond asynchronously to avoid recursion issues
        QTimer::singleShot(10, [this, msg]() { handleSentMessage(msg); });
    }

    void sendRawPairing(const QByteArray &data) override {
        qDebug().noquote() << "SENT_HEX:" << data.toHex().toUpper();
        emit rawPairingSent(data);

        // Simulate pairing status response
        QTimer::singleShot(10, [this]() {
            dji::Message resp;
            resp.subsystem = dji::SubsystemID::Status;
            resp.msgType = dji::MessageType::MaybeStatus;
            resp.payload = QByteArray(100, 0);
            simulateIncomingMessage(resp);
        });
    }

    bool isConnected() const override {
        return true;
    }
    bool isInitialized() const override {
        return true;
    }

    void simulateIncomingMessage(const dji::Message &msg) {
        qDebug().noquote() << "RECV_HEX:" << msg.serialize().toHex().toUpper();
        emit messageReceived(msg);
    }

signals:
    void messageSent(const dji::Message &msg);
    void rawPairingSent(const QByteArray &data);

private:
    void handleSentMessage(const dji::Message &msg) {

        dji::Message resp;
        resp.subsystem = msg.subsystem;
        resp.msgId = msg.msgId;
        bool shouldRespond = false;

        qDebug() << "MockDevice sent message type:" << static_cast<int>(msg.msgType)
                 << "Subsystem:" << static_cast<int>(msg.subsystem);

        if (msg.msgType == dji::MessageType::SetPairingPIN) {

            resp.msgType = dji::MessageType::PairingStatus;
            resp.payload = QByteArray::fromHex("0001");
            shouldRespond = true;

        } else if (msg.msgType == dji::MessageType::PrepareToLiveStream) {

            resp.msgType = dji::MessageType::PrepareToLiveStreamResult;
            resp.payload = QByteArray::fromHex("00");
            shouldRespond = true;

        } else if (msg.msgType == dji::MessageType::ConnectToWiFi) {

            resp.msgType = dji::MessageType::ConnectToWiFiResult;
            resp.payload = QByteArray::fromHex("0000");
            shouldRespond = true;

        } else if (msg.msgType == dji::MessageType::StartStopStreaming ||
                   msg.msgType == dji::MessageType::Configure ||
                   msg.msgType == dji::MessageType::ConfigureStreaming) {
            resp.msgType = msg.msgType;
            resp.payload = QByteArray::fromHex("00");

            if (msg.subsystem == dji::SubsystemID::Streamer) {
                if (msg.msgType == dji::MessageType::ConfigureStreaming) {
                    // ConfigureStreaming doesn't seem to have a specific result type in constants.h
                    // but let's assume it returns success
                    resp.msgType = dji::MessageType::StartStopStreamingResult;
                } else {
                    resp.msgType = dji::MessageType::StartStopStreamingResult;
                }
            }
            shouldRespond = true;

            qDebug() << "StartStopStreaming/Configure payload hex:" << msg.payload.toHex();

            if (msg.subsystem == dji::SubsystemID::Streamer && msg.payload.size() >= 1 &&
                static_cast<unsigned char>(msg.payload.at(0)) == 0x01 &&
                msg.msgType == dji::MessageType::StartStopStreaming) {
                qDebug() << "Condition met, will send StreamingStatus later. Payload size:"
                         << msg.payload.size()
                         << "First byte:" << static_cast<int>(msg.payload.at(0));

                dji::Message status;
                status.subsystem = dji::SubsystemID::Status;
                status.msgType = dji::MessageType::StreamingStatus;
                status.payload = QByteArray(21, 0);
                status.payload[20] = 100;

                QTimer::singleShot(50, [this, status]() { simulateIncomingMessage(status); });
            }
        }

        if (shouldRespond) {
            simulateIncomingMessage(resp);
        }
    }
};

void TestConnectWifiAndStreaming::testFullFlow() {
    MockDevice device;

    QSignalSpy spyReceiver(&device, &dji::Device::messageReceived);

    dji::Message initStatus;
    initStatus.subsystem = dji::SubsystemID::Status;
    initStatus.msgType = dji::MessageType::MaybeStatus;
    initStatus.payload = QByteArray(100, 0);
    device.simulateIncomingMessage(initStatus);

    dji::DeviceManager manager(&device);
    QSignalSpy spyFinished(&manager, &dji::DeviceManager::finished);

    dji::ConnectionOptions opts;
    opts.ssid = "test-ssid";
    opts.psk = "test-psk";
    opts.rtmpUrl = "rtmp://test/live";

    manager.connectToWiFiAndStartStreaming(&device, opts);

    // Wait for finished signal or timeout
    QVERIFY(spyFinished.wait(5000));

    bool ok = spyFinished.at(0).at(1).toBool();

    if (ok) {
        // Wait a bit for the StreamingStatus message to arrive
        QEventLoop waitLoop;
        QTimer::singleShot(200, &waitLoop, &QEventLoop::quit);
        waitLoop.exec();
    }

    bool streamingStatusReceived = false;
    for (int i = 0; i < spyReceiver.count(); ++i) {
        auto args = spyReceiver.at(i);
        dji::Message msg = args.at(0).value<dji::Message>();
        if (msg.msgType == dji::MessageType::StreamingStatus) {
            streamingStatusReceived = true;
            break;
        }
    }

    QVERIFY2(ok && streamingStatusReceived, "Flow failed or StreamingStatus not received");
}

#include "tst_connect_flow.moc"
