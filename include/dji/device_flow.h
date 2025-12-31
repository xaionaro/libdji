#ifndef DJI_DEVICE_FLOW_H
#define DJI_DEVICE_FLOW_H

#include "dji/device_manager.h"
#include <QObject>

namespace dji {

class Device;

/**
 * @brief Abstract base class for device operation flows.
 */
class DeviceFlow : public QObject {
    Q_OBJECT
public:
    explicit DeviceFlow(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~DeviceFlow() = default;

    virtual void start(Device *dev) = 0;
    virtual void stop() {}

signals:
    void finished(Device *dev, bool success);
    void log(const QString &msg);
};

/**
 * @brief Concrete flow for pairing, connecting to WiFi, and starting a live stream.
 */
class StreamingStarter : public DeviceFlow {
    Q_OBJECT
public:
    explicit StreamingStarter(const StreamingOptions &options, QObject *parent = nullptr);

    void start(Device *dev) override;
    void stop() override;

private slots:
    void onInitialized();
    void onPairingComplete();
    void onWifiConnected();
    void onPrepareComplete();
    void onStartComplete();
    void onError(const QString &msg);

private:
    Device *m_device = nullptr;
    StreamingOptions m_options;
};

} // namespace dji

#endif // DJI_DEVICE_FLOW_H
