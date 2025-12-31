#ifndef DJI_SUBSYSTEM_CONFIGURER_H
#define DJI_SUBSYSTEM_CONFIGURER_H

#include "dji/message.h"
#include <QByteArray>
#include <QObject>

namespace dji {

class Device;

class SubsystemConfigurer : public QObject {
    Q_OBJECT
public:
    explicit SubsystemConfigurer(Device *device);

    SubsystemID subsystemID() const {
        return SubsystemID::Configurer;
    }

    void setImageStabilization(ImageStabilization v);
    void handleMessage(const Message &msg);

signals:
    void log(const QString &message);
    void error(const QString &message);
    void imageStabilizationSet();

private:
    Device *m_device;

    void sendMessageSetImageStabilization(ImageStabilization v);
};

} // namespace dji

#endif
