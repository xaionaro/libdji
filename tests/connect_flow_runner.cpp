#include "connect_flow_runner.h"

#include <QEventLoop>
#include <QPointer>
#include <QTimer>

#include "dji/device.h"
#include "dji/subsystem_pairer.h"
#include "dji/subsystem_streamer.h"

namespace connect_flow {

namespace {

class FlowRunner : public QObject {
    Q_OBJECT
public:
    FlowRunner(dji::Device *device, const Options &opt) : m_device(device), m_options(opt) {
        connect(device, &dji::Device::connected, this, &FlowRunner::onConnected);
        connect(device, &dji::Device::disconnected, this, &FlowRunner::onDisconnected);
        connect(device, &dji::Device::errorOccurred, this, &FlowRunner::onError);

        auto pairer = device->pairer();
        auto streamer = device->streamer();

        connect(pairer, &dji::SubsystemPairer::pairingComplete, this,
                &FlowRunner::onPairingComplete);
        connect(pairer, &dji::SubsystemPairer::wifiConnected, this, &FlowRunner::onWifiConnected);
        connect(pairer, &dji::SubsystemPairer::error, this, &FlowRunner::onError);

        connect(streamer, &dji::SubsystemStreamer::prepareToLiveStreamComplete, this,
                &FlowRunner::onPrepareComplete);
        connect(streamer, &dji::SubsystemStreamer::startLiveStreamComplete, this,
                &FlowRunner::onStartComplete);
        connect(streamer, &dji::SubsystemStreamer::error, this, &FlowRunner::onError);
    }

    bool run(QString *errorOut) {
        if (m_options.initiateConnection) {
            m_device->connectToDevice();
            if (!waitForPhase(Phase::Connected, "Timeout waiting for device connection")) {
                if (errorOut)
                    *errorOut = m_error;
                return false;
            }
        } else {
            updatePhase(Phase::Connected);
        }

        m_device->pairer()->pair();
        if (!waitForPhase(Phase::Paired, "Timeout waiting for pairing")) {
            if (errorOut)
                *errorOut = m_error;
            return false;
        }

        m_device->streamer()->prepareToLiveStream();
        if (!waitForPhase(Phase::Prepared, "Timeout waiting for prepareToLiveStream")) {
            if (errorOut)
                *errorOut = m_error;
            return false;
        }

        m_device->pairer()->connectToWiFi(m_options.ssid, m_options.psk);
        if (!waitForPhase(Phase::WifiConnected, "Timeout waiting for WiFi connection")) {
            if (errorOut)
                *errorOut = m_error;
            return false;
        }

        m_device->streamer()->startLiveStream(m_options.resolution, m_options.bitrateKbps,
                                              m_options.fps, m_options.rtmpUrl);
        if (!waitForPhase(Phase::Started, "Timeout waiting for live stream start")) {
            if (errorOut)
                *errorOut = m_error;
            return false;
        }

        if (errorOut)
            *errorOut = m_error;
        return true;
    }

signals:
    void progress();

private:
    enum class Phase { Idle, Connected, Paired, Prepared, WifiConnected, Started };

    bool waitForPhase(Phase target, const QString &timeoutMessage) {
        if (m_phase >= target)
            return true;

        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        const int timeout = m_options.stepTimeoutMs > 0 ? m_options.stepTimeoutMs : 10000;

        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        connect(this, &FlowRunner::progress, &loop, &QEventLoop::quit);

        timer.start(timeout);
        while (m_phase < target && !m_failed) {
            if (!timer.isActive())
                break;
            loop.exec();
        }

        if (m_phase < target && !m_failed) {
            setError(timeoutMessage);
        }
        return !m_failed && m_phase >= target;
    }

    void updatePhase(Phase phase) {
        if (phase > m_phase) {
            m_phase = phase;
            emit progress();
        }
    }

    void onConnected() {
        updatePhase(Phase::Connected);
    }
    void onDisconnected() {
        setError("Device disconnected");
    }
    void onPairingComplete() {
        updatePhase(Phase::Paired);
    }
    void onPrepareComplete() {
        updatePhase(Phase::Prepared);
    }
    void onWifiConnected() {
        updatePhase(Phase::WifiConnected);
    }
    void onStartComplete() {
        updatePhase(Phase::Started);
    }
    void onError(const QString &msg) {
        setError(msg);
    }

    void setError(const QString &msg) {
        if (m_failed)
            return;
        m_failed = true;
        m_error = msg;
        emit progress();
    }

    dji::Device *m_device;
    Options m_options;
    Phase m_phase = Phase::Idle;
    bool m_failed = false;
    QString m_error;
};

} // namespace

bool runConnectWiFiAndStartStreaming(dji::Device *device, const Options &options,
                                     QString *errorOut) {
    if (!device) {
        if (errorOut)
            *errorOut = "Device pointer is null";
        return false;
    }
    FlowRunner runner(device, options);
    return runner.run(errorOut);
}

} // namespace connect_flow

#include "connect_flow_runner.moc"
