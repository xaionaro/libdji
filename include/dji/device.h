#ifndef DJI_DEVICE_H
#define DJI_DEVICE_H

#include "dji/message.h"
#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QObject>

namespace dji {

class SubsystemPairer;
class SubsystemStreamer;
class SubsystemConfigurer;

class Device : public QObject {
    Q_OBJECT
public:
    explicit Device(const QBluetoothDeviceInfo &info, DeviceType type, QObject *parent = nullptr);
    ~Device() override;

    void connectToDevice();
    void disconnectFromDevice();

    virtual void sendMessage(const Message &msg, bool noResponse = true);
    virtual void sendRawPairing(const QByteArray &data);

    DeviceType deviceType() const {
        return m_deviceType;
    }
    void setDeviceType(DeviceType t) {
        m_deviceType = t;
    }

    bool isConnected() const;
    bool isInitialized() const;

    SubsystemPairer *pairer() {
        return m_pairer;
    }
    SubsystemStreamer *streamer() {
        return m_streamer;
    }
    SubsystemConfigurer *configurer() {
        return m_configurer;
    }

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &message);
    void log(const QString &message);

    void messageReceived(const Message &msg);

private slots:
    void onControllerConnected();
    void onControllerDisconnected();
    void onControllerError(QLowEnergyController::Error error);
    void onServiceDiscovered(const QBluetoothUuid &newService);
    void onServiceDiscoveryFinished();
    void onServiceStateChanged(QLowEnergyService::ServiceState newState);
    void onCharacteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value);
    void onCharacteristicWriteFinished(const QLowEnergyCharacteristic &c, const QByteArray &value);
    void onCharacteristicReadFinished(const QLowEnergyCharacteristic &c, const QByteArray &value);

private:
    void discoverCharacteristics();
    void receiveNotification(const QByteArray &data);

    SubsystemPairer *m_pairer;
    SubsystemStreamer *m_streamer;
    SubsystemConfigurer *m_configurer;
    DeviceType m_deviceType = DeviceType::Unknown;

    QBluetoothDeviceInfo m_deviceInfo;
    QLowEnergyController *m_controller = nullptr;
    QLowEnergyService *m_service = nullptr;

    QLowEnergyCharacteristic m_charReceiver;
    QLowEnergyCharacteristic m_charSender;
    QLowEnergyCharacteristic m_charPairingRequestor;

    bool m_initialized = false;
};

} // namespace dji

#endif
