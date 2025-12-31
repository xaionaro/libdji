/**
 * @file device_manager.h
 * @brief Centralized manager for the DJI connection and streaming flow.
 *
 * This class implements a state machine to handle the sequence of pairing,
 * preparing, WiFi connection, and starting the live stream.
 */

#ifndef DJI_DEVICE_MANAGER_H
#define DJI_DEVICE_MANAGER_H

#include "dji/constants.h"
#include <QBluetoothDeviceInfo>
#include <QObject>
#include <QString>

class QBluetoothDeviceDiscoveryAgent;

namespace dji {

class Device;
class DeviceFlow;

struct ConnectionOptions {
    QString ssid;
    QString psk;
    QString rtmpUrl;
    Resolution resolution = Resolution::Res1080p;
    uint16_t bitrateKbps = 4000;
    FPS fps = FPS::FPS25;
    QString deviceAddrFilter;
    QString deviceNameFilter;
};

class DeviceManager : public QObject {
    Q_OBJECT
public:
    explicit DeviceManager(Device *device = nullptr, QObject *parent = nullptr);
    ~DeviceManager();

    void connectToWiFiAndStartStreaming(Device *dev, const ConnectionOptions &options);
    void runFlow(Device *dev, DeviceFlow *flow);
    void startDiscovery(const ConnectionOptions &options = ConnectionOptions());
    void stopDiscovery();
    void stop();

    bool isPaired(Device *dev = nullptr) const;
    bool isWiFiConnected(Device *dev = nullptr) const;
    bool isStreaming(Device *dev = nullptr) const;
    Device *device() const;
    QList<Device *> devices() const {
        return m_devices;
    }

signals:
    void isPairedChanged(Device *device);
    void isWiFiConnectedChanged(Device *device);
    void isStreamingChanged(Device *device);
    void deviceChanged();
    void devicesChanged();
    void log(const QString &message);
    void error(const QString &message);
    void finished(Device *device, bool success);

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &info);
    void onScanFinished();
    void onScanError();
    void onPairingComplete(Device *dev);
    void onWifiConnected(Device *dev);
    void onPrepareComplete(Device *dev);
    void onStartComplete(Device *dev);
    void onStopComplete(Device *dev);
    void onError(Device *dev, const QString &msg);

private:
    struct DeviceState {
        bool isPaired = false;
        bool isWiFiConnected = false;
        bool isStreaming = false;
        bool isPrepared = false;
        DeviceFlow *activeFlow = nullptr;
    };

    void addDevice(Device *device);

    QList<Device *> m_devices;
    QMap<Device *, DeviceState> m_deviceStates;
    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent = nullptr;
    ConnectionOptions m_options;
};

} // namespace dji

#endif
