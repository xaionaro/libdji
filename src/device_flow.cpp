#include "dji/device_flow.h"
#include "dji/device.h"
#include "dji/subsystem_pairer.h"
#include "dji/subsystem_streamer.h"

namespace dji {

StreamingStarter::StreamingStarter(const StreamingOptions &options, QObject *parent)
    : DeviceFlow(parent), m_options(options) {}

void StreamingStarter::start(Device *dev) {
    m_device = dev;

    connect(dev, &Device::initialized, this, &StreamingStarter::onInitialized);
    connect(dev->pairer(), &SubsystemPairer::pairingComplete, this,
            &StreamingStarter::onPairingComplete);
    connect(dev->pairer(), &SubsystemPairer::wifiConnected, this,
            &StreamingStarter::onWifiConnected);
    connect(dev->pairer(), &SubsystemPairer::error, this, &StreamingStarter::onError);

    connect(dev->streamer(), &SubsystemStreamer::prepareToLiveStreamComplete, this,
            &StreamingStarter::onPrepareComplete);
    connect(dev->streamer(), &SubsystemStreamer::startLiveStreamComplete, this,
            &StreamingStarter::onStartComplete);
    connect(dev->streamer(), &SubsystemStreamer::error, this, &StreamingStarter::onError);

    if (dev->isInitialized()) {
        onInitialized();
    } else if (!dev->isConnected()) {
        emit log(QString("[DJI-BLE] Flow: Connecting to device %1...")
                     .arg(dev->deviceInfo().address().toString()));
        dev->connectToDevice();
    } else {
        emit log(QString("[DJI-BLE] Flow: Waiting for device %1 initialization...")
                     .arg(dev->deviceInfo().address().toString()));
    }
}

void StreamingStarter::stop() {
    if (m_device) {
        m_device->streamer()->stopLiveStream();
    }
}

void StreamingStarter::onInitialized() {
    emit log(QString("[DJI-BLE] Flow: Device %1 initialized. Starting pairing...")
                 .arg(m_device->deviceInfo().address().toString()));
    m_device->pairer()->pair();
}

void StreamingStarter::onPairingComplete() {
    emit log(QString("[DJI-BLE] Flow: Pairing complete for %1. Preparing to live stream...")
                 .arg(m_device->deviceInfo().address().toString()));
    m_device->streamer()->prepareToLiveStream();
}

void StreamingStarter::onPrepareComplete() {
    emit log(QString("[DJI-BLE] Flow: Prepare complete for %1. Connecting to WiFi %2...")
                 .arg(m_device->deviceInfo().address().toString(), m_options.ssid));
    m_device->pairer()->connectToWiFi(m_options.ssid, m_options.psk);
}

void StreamingStarter::onWifiConnected() {
    emit log(QString("[DJI-BLE] Flow: WiFi connected for %1. Starting live stream...")
                 .arg(m_device->deviceInfo().address().toString()));
    m_device->streamer()->startLiveStream(m_options.resolution, m_options.bitrateKbps, m_options.fps,
                                     m_options.rtmpUrl);
}

void StreamingStarter::onStartComplete() {
    emit log(QString("[DJI-BLE] Flow: Live stream started for %1.")
                 .arg(m_device->deviceInfo().address().toString()));
    emit finished(m_device, true);
}

void StreamingStarter::onError(const QString &msg) {
    emit log(QString("[DJI-BLE] Flow error: %1").arg(msg));
    emit finished(m_device, false);
}

} // namespace dji
