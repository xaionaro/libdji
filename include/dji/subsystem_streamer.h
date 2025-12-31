#ifndef DJI_SUBSYSTEM_STREAMER_H
#define DJI_SUBSYSTEM_STREAMER_H

#include "dji/message.h"
#include <QByteArray>
#include <QObject>

namespace dji {

class Device;

class SubsystemStreamer : public QObject {
    Q_OBJECT
public:
    explicit SubsystemStreamer(Device *device);

    SubsystemID subsystemID() const {
        return SubsystemID::Streamer;
    }

    void prepareToLiveStream();
    void startLiveStream(Resolution resolution, uint16_t bitrateKbps, FPS fps,
                         const QString &rtmpURL);
    void stopLiveStream();

    void handleMessage(const Message &msg);

signals:
    void prepareToLiveStreamComplete();
    void startLiveStreamComplete();
    void stopLiveStreamComplete();
    void batteryPercentageChanged(int percentage);
    void error(const QString &message);
    void log(const QString &message);

private:
    enum class State { Idle, PreparingStage1, PreparingStage2, Starting, Stopping };
    Device *m_device;
    State m_state = State::Idle;

    Resolution m_pendingResolution;
    uint16_t m_pendingBitrate;
    FPS m_pendingFps;
    QString m_pendingRtmpUrl;

    void sendMessagePrepareToLiveStreamStage1();
    void sendMessagePrepareToLiveStreamStage2();
    void sendMessageConfigureLiveStream(Resolution resolution, uint16_t bitrateKbps, FPS fps,
                                        const QString &rtmpURL);
    void sendMessageStartLiveStream();
    void sendMessageStopLiveStream();
};

} // namespace dji

#endif
