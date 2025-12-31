/**
 * @file device.h
 * @brief Base class for DJI devices, providing BLE communication and subsystem access.
 */

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
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString address READ address NOTIFY addressChanged)
    Q_PROPERTY(DeviceType deviceType READ deviceType NOTIFY deviceTypeChanged)
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connected)
    Q_PROPERTY(bool isInitialized READ isInitialized NOTIFY initialized)

public:
    explicit Device(QObject *parent = nullptr);
    explicit Device(const QBluetoothDeviceInfo &info, DeviceType type, QObject *parent = nullptr);
    ~Device() override;

    virtual void connectToDevice();
    virtual void disconnectFromDevice();

    virtual void sendMessage(const Message &msg, bool noResponse = true);
    virtual void sendRawPairing(const QByteArray &data);

    QString name() const {
        return m_deviceInfo.name();
    }
    QString address() const {
        return m_deviceInfo.address().toString();
    }

    DeviceType deviceType() const {
        return m_deviceType;
    }
    void setDeviceType(DeviceType t) {
        if (m_deviceType != t) {
            m_deviceType = t;
            emit deviceTypeChanged();
        }
    }

    QBluetoothDeviceInfo deviceInfo() const {
        return m_deviceInfo;
    }

    virtual bool isConnected() const;
    virtual bool isInitialized() const;

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
    void initialized();
    void errorOccurred(const QString &message);
    void log(const QString &message);

    void messageReceived(const Message &msg);
    void nameChanged();
    void addressChanged();
    void deviceTypeChanged();

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

protected:
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
