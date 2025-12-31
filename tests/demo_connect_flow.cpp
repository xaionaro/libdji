#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

#include "dji/constants.h"
#include "dji/device.h"
#include "dji/device_manager.h"

class DemoController : public QObject {
    Q_OBJECT
public:
    DemoController(dji::StreamingOptions opts, int scanTimeoutMs, QObject *parent = nullptr)
        : QObject(parent), m_options(std::move(opts)),
          m_scanTimeout(scanTimeoutMs > 0 ? scanTimeoutMs : 30000) {
        m_manager = new dji::DeviceManager(nullptr, this);
        connect(m_manager, &dji::DeviceManager::log, this, &DemoController::onLog);
        connect(m_manager, &dji::DeviceManager::error, this, &DemoController::onError);
        connect(m_manager, &dji::DeviceManager::finished, this, &DemoController::onFinished);
        connect(m_manager, &dji::DeviceManager::deviceChanged, this, [this]() {
            if (m_manager->device() && !m_started) {
                m_started = true;
                m_manager->connectToWiFiAndStartStreaming(m_manager->device(), m_options);
            }
        });
    }

    void start(const dji::DiscoveryOptions &discOpts) {
        qInfo() << "Starting discovery...";
        m_manager->startDiscovery(discOpts);
        QTimer::singleShot(m_scanTimeout, this, [this]() {
            if (!m_manager->device()) {
                fail("Discovery timed out without matching device");
            }
        });
    }

private slots:
    void onLog(const QString &message) {
        qInfo().noquote() << message;
    }

    void onError(const QString &message) {
        fail(message);
    }

    void onFinished(dji::Device *dev, bool success) {
        Q_UNUSED(dev);
        if (success) {
            qInfo() << "Flow completed successfully";
            QCoreApplication::quit();
        } else {
            fail("Flow failed");
        }
    }

private:
    void fail(const QString &message) {
        if (m_failed)
            return;
        m_failed = true;
        qCritical().noquote() << message;
        QCoreApplication::exit(1);
    }

    dji::DeviceManager *m_manager = nullptr;
    dji::StreamingOptions m_options;
    int m_scanTimeout = 30000;
    bool m_failed = false;
    bool m_started = false;
};

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("libdji-demo");
    QCoreApplication::setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption ssidOpt("wifi-ssid", "WiFi SSID", "ssid");
    QCommandLineOption pskOpt("wifi-psk", "WiFi PSK", "psk");
    QCommandLineOption rtmpOpt("rtmp-url", "RTMP URL", "url");
    QCommandLineOption filterAddrOpt("filter-device-addr", "Filter by device address substring",
                                     "addr");
    QCommandLineOption filterNameOpt("filter-device-name", "Filter by device name substring",
                                     "name");
    QCommandLineOption scanTimeoutOpt("scan-timeout", "Scan timeout seconds", "seconds", "30");

    parser.addOption(ssidOpt);
    parser.addOption(pskOpt);
    parser.addOption(rtmpOpt);
    parser.addOption(filterAddrOpt);
    parser.addOption(filterNameOpt);
    parser.addOption(scanTimeoutOpt);

    parser.process(app);

    if (!parser.isSet(ssidOpt) || !parser.isSet(pskOpt) || !parser.isSet(rtmpOpt)) {
        parser.showHelp(1);
    }

    dji::StreamingOptions opts;
    opts.ssid = parser.value(ssidOpt);
    opts.psk = parser.value(pskOpt);
    opts.rtmpUrl = parser.value(rtmpOpt);

    dji::DiscoveryOptions discOpts;
    discOpts.deviceAddrFilter = parser.value(filterAddrOpt);
    discOpts.deviceNameFilter = parser.value(filterNameOpt);

    bool ok = false;
    int scanTimeout = parser.value(scanTimeoutOpt).toInt(&ok);

    DemoController controller(opts, ok ? scanTimeout * 1000 : 30000);
    controller.start(discOpts);

    return app.exec();
}

#include "demo_connect_flow.moc"
