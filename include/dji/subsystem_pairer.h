#ifndef DJI_SUBSYSTEM_PAIRER_H
#define DJI_SUBSYSTEM_PAIRER_H

#include "dji/message.h"
#include <QByteArray>
#include <QObject>

namespace dji {

class Device;

class SubsystemPairer : public QObject {
    Q_OBJECT
public:
    enum class State { Idle, WaitingForStatus, WaitingForApproval, Finalizing };

    explicit SubsystemPairer(Device *device);

    SubsystemID subsystemID() const {
        return SubsystemID::Pairer;
    }

    void pair();
    void connectToWiFi(const QString &ssid, const QString &psk);
    void startScanningWiFi();
    void handleMessage(const Message &msg);

    State state() const {
        return m_state;
    }

signals:
    void pairingComplete();
    void wifiConnected();
    void wifiScanReport(const QByteArray &report);
    void error(const QString &message);
    void log(const QString &message);

private:
    Device *m_device;
    State m_state = State::Idle;

    void sendRequestStartPairing();
    void sendMessageSetPairingPIN(const QString &pinCode);
    void sendMessagePairingStage1();
    void sendMessagePairingStage2();

    void sendMessageStartScanningWiFi();
    void sendMessageConnectToWiFi(const QString &ssid, const QString &psk);
};

} // namespace dji

#endif
