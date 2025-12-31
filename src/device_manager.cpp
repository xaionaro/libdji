/**
 * @file device_manager.cpp
 * @brief Implementation of the DJI connection and streaming state machine.
 */

#include "dji/device_manager.h"
#include "dji/device.h"
#include "dji/device_flow.h"
#include "dji/subsystem_pairer.h"
#include "dji/subsystem_streamer.h"
#include <QBluetoothDeviceDiscoveryAgent>
#include <QDebug>

namespace dji {

DeviceManager::DeviceManager(Device *device, QObject *parent) : QObject(parent) {
    if (device) {
        addDevice(device);
    }
}

DeviceManager::~DeviceManager() {
    stopDiscovery();
}

Device *DeviceManager::device() const {
    return m_devices.isEmpty() ? nullptr : m_devices.first();
}

bool DeviceManager::isPaired(Device *dev) const {
    if (!dev)
        dev = device();
    return dev ? m_deviceStates.value(dev).isPaired : false;
}

bool DeviceManager::isWiFiConnected(Device *dev) const {
    if (!dev)
        dev = device();
    return dev ? m_deviceStates.value(dev).isWiFiConnected : false;
}

bool DeviceManager::isStreaming(Device *dev) const {
    if (!dev)
        dev = device();
    return dev ? m_deviceStates.value(dev).isStreaming : false;
}

void DeviceManager::addDevice(Device *device) {
    if (!device || m_devices.contains(device))
        return;

    m_devices.append(device);
    m_deviceStates[device] = DeviceState();

    connect(device, &Device::disconnected, this, [this, device]() {
        if (m_deviceStates.contains(device) && m_deviceStates[device].activeFlow) {
            onError(device, "Device disconnected during flow");
        }
    });
    connect(device, &Device::errorOccurred, this,
            [this, device](const QString &msg) { onError(device, msg); });

    connect(device->pairer(), &SubsystemPairer::pairingComplete, this,
            [this, device]() { onPairingComplete(device); });
    connect(device->pairer(), &SubsystemPairer::wifiConnected, this,
            [this, device]() { onWifiConnected(device); });
    connect(device->pairer(), &SubsystemPairer::error, this,
            [this, device](const QString &msg) { onError(device, msg); });
    connect(device->pairer(), &SubsystemPairer::log, this, &DeviceManager::log);

    connect(device->streamer(), &SubsystemStreamer::prepareToLiveStreamComplete, this,
            [this, device]() { onPrepareComplete(device); });
    connect(device->streamer(), &SubsystemStreamer::startLiveStreamComplete, this,
            [this, device]() { onStartComplete(device); });
    connect(device->streamer(), &SubsystemStreamer::stopLiveStreamComplete, this,
            [this, device]() { onStopComplete(device); });
    connect(device->streamer(), &SubsystemStreamer::error, this,
            [this, device](const QString &msg) { onError(device, msg); });
    connect(device->streamer(), &SubsystemStreamer::log, this, &DeviceManager::log);

    emit deviceChanged();
    emit devicesChanged();
}

void DeviceManager::startDiscovery(const ConnectionOptions &options) {
    m_options = options;
    if (!m_discoveryAgent) {
        m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
        connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this,
                &DeviceManager::onDeviceDiscovered);
        connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceUpdated, this,
                [this](const QBluetoothDeviceInfo &info, QBluetoothDeviceInfo::Fields fields) {
                    Q_UNUSED(fields);
                    onDeviceDiscovered(info);
                });
        connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this,
                &DeviceManager::onScanFinished);
        connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred, this,
                &DeviceManager::onScanError);
    }
    emit log("[DJI-BLE] Manager: Starting device discovery...");
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void DeviceManager::stopDiscovery() {
    if (m_discoveryAgent && m_discoveryAgent->isActive()) {
        m_discoveryAgent->stop();
    }
}

void DeviceManager::onDeviceDiscovered(const QBluetoothDeviceInfo &info) {
    for (auto dev : m_devices) {
        if (dev->deviceInfo().address() == info.address()) {
            return;
        }
    }

    qDebug() << "[DJI-BLE] Discovered device" << info.name() << info.address().toString();
    auto manufacturerData = info.manufacturerData();
    qDebug() << "[DJI-BLE] Manufacturer data keys:" << manufacturerData.keys();
    for (auto key : manufacturerData.keys()) {
        qDebug() << "[DJI-BLE] Manufacturer data for" << QString("0x%1").arg(key, 4, 16, QChar('0'))
                 << ":" << manufacturerData.value(key).toHex();
    }

    DeviceType deviceType = identifyDeviceType(manufacturerData.value(0x08AA));

    if (deviceType == DeviceType::Undefined && !m_options.deviceNameFilter.isEmpty()) {
        if (info.name().contains(m_options.deviceNameFilter, Qt::CaseInsensitive)) {
            deviceType = DeviceType::Unknown;
        }
    }

    if (deviceType == DeviceType::Undefined) {
        return;
    }

    if (!m_options.deviceAddrFilter.isEmpty() &&
        !info.address().toString().contains(m_options.deviceAddrFilter, Qt::CaseInsensitive)) {
        return;
    }

    emit log(QString("[DJI-BLE] Manager: Matched DJI device %1 (%2) type: %3")
                 .arg(info.name(), info.address().toString())
                 .arg(static_cast<int>(deviceType)));

    Device *dev = new Device(info, deviceType, this);
    addDevice(dev);
}

void DeviceManager::onScanFinished() {
    emit log("[DJI-BLE] Manager: Device discovery finished.");
}

void DeviceManager::onScanError() {
    onError(nullptr, QString("Discovery error: %1").arg(m_discoveryAgent->errorString()));
}

void DeviceManager::connectToWiFiAndStartStreaming(Device *dev, const ConnectionOptions &options) {
    runFlow(dev, new StreamingStarter(options, this));
}

void DeviceManager::runFlow(Device *dev, DeviceFlow *flow) {
    if (!dev || !flow)
        return;
    if (!m_devices.contains(dev)) {
        addDevice(dev);
    }

    DeviceState &state = m_deviceStates[dev];
    if (state.activeFlow) {
        state.activeFlow->deleteLater();
    }
    state.activeFlow = flow;

    connect(flow, &DeviceFlow::log, this, &DeviceManager::log);
    connect(flow, &DeviceFlow::finished, this, [this, dev](Device *d, bool success) {
        if (d == dev) {
            m_deviceStates[dev].activeFlow = nullptr;
            sender()->deleteLater();
            emit finished(dev, success);
        }
    });

    flow->start(dev);
}

void DeviceManager::stop() {
    stopDiscovery();
    for (auto dev : m_devices) {
        DeviceState &state = m_deviceStates[dev];
        if (state.activeFlow) {
            state.activeFlow->stop();
            state.activeFlow->deleteLater();
            state.activeFlow = nullptr;
        }
    }
}

void DeviceManager::onPairingComplete(Device *dev) {
    m_deviceStates[dev].isPaired = true;
    emit isPairedChanged(dev);
}

void DeviceManager::onWifiConnected(Device *dev) {
    m_deviceStates[dev].isWiFiConnected = true;
    emit isWiFiConnectedChanged(dev);
}

void DeviceManager::onPrepareComplete(Device *dev) {
    m_deviceStates[dev].isPrepared = true;
}

void DeviceManager::onStartComplete(Device *dev) {
    m_deviceStates[dev].isStreaming = true;
    emit isStreamingChanged(dev);
}

void DeviceManager::onStopComplete(Device *dev) {
    m_deviceStates[dev].isStreaming = false;
    emit isStreamingChanged(dev);
}

void DeviceManager::onError(Device *dev, const QString &msg) {
    emit error(QString("[DJI-BLE] %1: %2")
                   .arg(dev ? dev->deviceInfo().address().toString() : "Manager", msg));
}

} // namespace dji
