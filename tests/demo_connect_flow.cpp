#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

#include "connect_flow_runner.h"
#include "dji/constants.h"
#include "dji/device.h"

class DemoController : public QObject {
    Q_OBJECT
public:
    DemoController(connect_flow::Options flowOpts, const QString &deviceAddrFilter,
                   int scanTimeoutMs, QObject *parent = nullptr)
        : QObject(parent), m_flowOptions(std::move(flowOpts)), m_deviceAddrFilter(deviceAddrFilter),
          m_scanTimeout(scanTimeoutMs > 0 ? scanTimeoutMs : 30000) {
        m_agent = new QBluetoothDeviceDiscoveryAgent(this);
        connect(m_agent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this,
                &DemoController::onDeviceDiscovered);
        connect(m_agent, &QBluetoothDeviceDiscoveryAgent::finished, this,
                &DemoController::onScanFinished);
        connect(m_agent, &QBluetoothDeviceDiscoveryAgent::errorOccurred, this,
                &DemoController::onScanError);
    }

    void start() {
        qInfo() << "Starting BLE scan...";
        m_agent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
        QTimer::singleShot(m_scanTimeout, this, [this]() {
            if (!m_device) {
                fail("Scan timed out without matching device");
            }
        });
    }

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &info) {
        qInfo() << "Discovered device" << info.name() << info.address().toString();
        auto manufacturerData = info.manufacturerData();
        qDebug() << "Manufacturer data keys:" << manufacturerData.keys();
        for (auto key : manufacturerData.keys()) {
            qDebug() << "Manufacturer data for" << QString("0x%1").arg(key, 4, 16, QChar('0'))
                     << ":" << manufacturerData.value(key).toHex();
        }
        dji::DeviceType deviceType = dji::identifyDeviceType(info.manufacturerData().value(0x08AA));
        if (deviceType == dji::DeviceType::Undefined) {
            return;
        }
        if (!m_deviceAddrFilter.isEmpty() &&
            !info.address().toString().contains(m_deviceAddrFilter, Qt::CaseInsensitive)) {
            qInfo() << "Skipping device" << info.address().toString()
                    << "because address does not match filter" << m_deviceAddrFilter;
            return;
        }
        qInfo() << "Matched DJI device" << info.name() << info.address().toString()
                << "type:" << static_cast<int>(deviceType);
        m_agent->stop();
        createDevice(info, deviceType);
    }

    void onScanFinished() {
        if (!m_device) {
            fail("Scan finished without matching device");
        }
    }

    void onScanError(QBluetoothDeviceDiscoveryAgent::Error error) {
        Q_UNUSED(error);
        fail(QStringLiteral("Scan error: %1").arg(m_agent->errorString()));
    }

    void onDeviceLog(const QString &message) {
        qInfo().noquote() << message;
    }

    void onDeviceError(const QString &message) {
        fail(QStringLiteral("Device error: %1").arg(message));
    }

private:
    void createDevice(const QBluetoothDeviceInfo &info, dji::DeviceType type) {
        if (m_device)
            return;
        m_device = new dji::Device(info, type, this);
        connect(m_device, &dji::Device::log, this, &DemoController::onDeviceLog);
        connect(m_device, &dji::Device::errorOccurred, this, &DemoController::onDeviceError);

        QString error;
        if (!connect_flow::runConnectWiFiAndStartStreaming(m_device, m_flowOptions, &error)) {
            fail(error.isEmpty() ? QStringLiteral("Flow failed") : error);
            return;
        }
        qInfo() << "Flow completed successfully";
        QCoreApplication::quit();
    }

    void fail(const QString &message) {
        if (m_failed)
            return;
        m_failed = true;
        qCritical().noquote() << message;
        QCoreApplication::exit(1);
    }

    QBluetoothDeviceDiscoveryAgent *m_agent = nullptr;
    dji::Device *m_device = nullptr;
    connect_flow::Options m_flowOptions;
    QString m_deviceAddrFilter;
    int m_scanTimeout = 30000;
    bool m_failed = false;
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
    QCommandLineOption scanTimeoutOpt("scan-timeout", "Scan timeout seconds", "seconds", "30");

    parser.addOption(ssidOpt);
    parser.addOption(pskOpt);
    parser.addOption(rtmpOpt);
    parser.addOption(filterAddrOpt);
    parser.addOption(scanTimeoutOpt);

    parser.process(app);

    if (!parser.isSet(ssidOpt) || !parser.isSet(pskOpt) || !parser.isSet(rtmpOpt)) {
        parser.showHelp(1);
    }

    connect_flow::Options opts;
    opts.ssid = parser.value(ssidOpt);
    opts.psk = parser.value(pskOpt);
    opts.rtmpUrl = parser.value(rtmpOpt);
    opts.initiateConnection = true;

    bool ok = false;
    int scanTimeout = parser.value(scanTimeoutOpt).toInt(&ok);

    DemoController controller(opts, parser.value(filterAddrOpt), ok ? scanTimeout * 1000 : 30000);
    controller.start();

    return app.exec();
}

#include "demo_connect_flow.moc"
