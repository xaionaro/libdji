#include "dji/device.h"
#include "dji/subsystem_configurer.h"
#include "dji/subsystem_pairer.h"
#include "dji/subsystem_streamer.h"
#include <QDebug>
#include <QTimer>

namespace dji {

static const uint16_t characteristicIDReceiver = 0xfff4;
static const uint16_t characteristicIDPairingRequestor = 0xfff3;
static const uint16_t characteristicIDSender = 0xfff5;

Device::Device(const QBluetoothDeviceInfo &info, DeviceType type, QObject *parent)
    : QObject(parent), m_deviceInfo(info), m_deviceType(type) {
    m_pairer = new SubsystemPairer(this);
    m_streamer = new SubsystemStreamer(this);
    m_configurer = new SubsystemConfigurer(this);

    connect(m_pairer, &SubsystemPairer::log, this, &Device::log);
    connect(m_pairer, &SubsystemPairer::error, this, &Device::errorOccurred);
    connect(m_streamer, &SubsystemStreamer::log, this, &Device::log);
    connect(m_streamer, &SubsystemStreamer::error, this, &Device::errorOccurred);
    connect(m_configurer, &SubsystemConfigurer::log, this, &Device::log);
    connect(m_configurer, &SubsystemConfigurer::error, this, &Device::errorOccurred);

    connect(this, &Device::messageReceived, m_pairer, &SubsystemPairer::handleMessage);
    connect(this, &Device::messageReceived, m_streamer, &SubsystemStreamer::handleMessage);
    connect(this, &Device::messageReceived, m_configurer, &SubsystemConfigurer::handleMessage);
}

Device::~Device() {
    if (m_controller) {
        m_controller->disconnectFromDevice();
        delete m_controller;
    }
}

void Device::connectToDevice() {
    if (m_controller) {
        m_controller->disconnectFromDevice();
        delete m_controller;
    }

    m_controller = QLowEnergyController::createCentral(m_deviceInfo, this);
    connect(m_controller, &QLowEnergyController::connected, this, &Device::onControllerConnected);
    connect(m_controller, &QLowEnergyController::disconnected, this,
            &Device::onControllerDisconnected);
    connect(m_controller, &QLowEnergyController::errorOccurred, this, &Device::onControllerError);
    connect(m_controller, &QLowEnergyController::serviceDiscovered, this,
            &Device::onServiceDiscovered);
    connect(m_controller, &QLowEnergyController::discoveryFinished, this,
            &Device::onServiceDiscoveryFinished);

    emit log(QString("Connecting to %1 (%2)")
                 .arg(m_deviceInfo.name(), m_deviceInfo.address().toString()));
    m_controller->connectToDevice();
}

void Device::disconnectFromDevice() {
    if (m_controller) {
        m_controller->disconnectFromDevice();
    }
}

void Device::onControllerConnected() {
    emit log("Controller connected. Discovering services...");
    m_controller->discoverServices();
}

void Device::onControllerDisconnected() {
    emit log("Controller disconnected");
    emit disconnected();
    m_initialized = false;
}

void Device::onControllerError(QLowEnergyController::Error error) {
    emit errorOccurred(QString("Controller error: %1").arg(error));
}

void Device::onServiceDiscovered(const QBluetoothUuid &newService) {

    Q_UNUSED(newService);
}

void Device::onServiceDiscoveryFinished() {
    emit log("Service discovery finished. Searching for DJI characteristics...");
    discoverCharacteristics();
}

void Device::discoverCharacteristics() {
    auto services = m_controller->services();
    for (auto serviceUuid : services) {
        QLowEnergyService *service = m_controller->createServiceObject(serviceUuid, this);
        if (!service)
            continue;

        connect(service, &QLowEnergyService::stateChanged, this, &Device::onServiceStateChanged);
        connect(service, &QLowEnergyService::characteristicChanged, this,
                &Device::onCharacteristicChanged);
        connect(service, &QLowEnergyService::characteristicWritten, this,
                &Device::onCharacteristicWriteFinished);
        connect(service, &QLowEnergyService::characteristicRead, this,
                &Device::onCharacteristicReadFinished);

        service->discoverDetails();
    }
}

void Device::onServiceStateChanged(QLowEnergyService::ServiceState newState) {
    if (newState != QLowEnergyService::RemoteServiceDiscovered)
        return;

    QLowEnergyService *service = qobject_cast<QLowEnergyService *>(sender());
    if (!service)
        return;

    emit log(QString("Service %1 discovered with %2 characteristics")
                 .arg(service->serviceUuid().toString())
                 .arg(service->characteristics().size()));

    const QList<QLowEnergyCharacteristic> chars = service->characteristics();
    for (const QLowEnergyCharacteristic &c : chars) {
        QString props;
        if (c.properties() & QLowEnergyCharacteristic::Read)
            props += "Read ";
        if (c.properties() & QLowEnergyCharacteristic::Write)
            props += "Write ";
        if (c.properties() & QLowEnergyCharacteristic::WriteNoResponse)
            props += "WriteNoResp ";
        if (c.properties() & QLowEnergyCharacteristic::Notify)
            props += "Notify ";
        if (c.properties() & QLowEnergyCharacteristic::Indicate)
            props += "Indicate ";
        emit log(QString("Characteristic: %1 Properties: %2").arg(c.uuid().toString()).arg(props));

        bool isReceiver = false;
        bool isSender = false;
        bool isPairing = false;

        if (c.uuid() == QBluetoothUuid(static_cast<uint16_t>(characteristicIDReceiver))) {
            isReceiver = true;
        } else if (c.uuid() == QBluetoothUuid(static_cast<uint16_t>(characteristicIDSender))) {
            isSender = true;
        } else if (c.uuid() ==
                   QBluetoothUuid(static_cast<uint16_t>(characteristicIDPairingRequestor))) {
            isPairing = true;
        }

        if (isReceiver) {
            m_charReceiver = c;

            QLowEnergyDescriptor desc =
                c.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
            if (desc.isValid()) {
                service->writeDescriptor(desc, QByteArray::fromHex("0100"));
            }
            emit log(QString("Found Receiver characteristic: %1").arg(c.uuid().toString()));
        } else if (isSender) {
            m_charSender = c;
            emit log(QString("Found Sender characteristic: %1").arg(c.uuid().toString()));
        } else if (isPairing) {
            m_charPairingRequestor = c;
            emit log(QString("Found PairingRequestor characteristic: %1").arg(c.uuid().toString()));
        }
    }

    if (m_charReceiver.isValid() && m_charSender.isValid() && m_charPairingRequestor.isValid()) {
        if (!m_initialized) {
            m_initialized = true;
            m_service = service;

            emit log("Device initialized. All characteristics found.");
            emit connected();
        }
    }
}

void Device::onCharacteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value) {
    if (c.uuid() == m_charReceiver.uuid()) {
        receiveNotification(value);
    }
}

void Device::onCharacteristicWriteFinished(const QLowEnergyCharacteristic &c,
                                           const QByteArray &value) {
    Q_UNUSED(c);
    Q_UNUSED(value);
}

void Device::onCharacteristicReadFinished(const QLowEnergyCharacteristic &c,
                                          const QByteArray &value) {
    Q_UNUSED(c);
    Q_UNUSED(value);
}

void Device::receiveNotification(const QByteArray &data) {
    emit log(QString("Received notification: %1").arg(QString(data.toHex())));

    bool ok = false;
    Message msg = Message::parse(data, &ok);
    if (!ok) {
        emit log(QString("Failed to parse incoming message: %1").arg(QString(data.toHex())));
        return;
    }

    emit log(QString("Parsed message: subsystem=0x%1 id=0x%2 type=0x%3")
                 .arg(static_cast<uint16_t>(msg.subsystem), 0, 16)
                 .arg(static_cast<uint16_t>(msg.msgId), 0, 16)
                 .arg(static_cast<uint32_t>(msg.msgType), 0, 16));
    emit messageReceived(msg);
}

void Device::sendMessage(const Message &msg, bool noResponse) {
    if (!m_initialized || !m_service || !m_charSender.isValid()) {
        emit errorOccurred("Cannot send message: Device not initialized");
        return;
    }

    QByteArray data = msg.serialize();
    QLowEnergyService::WriteMode mode =
        noResponse ? QLowEnergyService::WriteWithoutResponse : QLowEnergyService::WriteWithResponse;
    m_service->writeCharacteristic(m_charSender, data, mode);
}

void Device::sendRawPairing(const QByteArray &data) {
    if (!m_initialized || !m_service || !m_charReceiver.isValid()) {
        emit errorOccurred("Cannot send pairing request: Device not initialized");
        return;
    }

    QLowEnergyDescriptor desc = m_charReceiver.descriptor(
        QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
    if (desc.isValid()) {
        emit log(QString("Sending raw pairing (writing to CCCD): %1").arg(QString(data.toHex())));
        m_service->writeDescriptor(desc, data);
    } else {
        emit log("Cannot send raw pairing: CCCD not found on Receiver characteristic");
    }
}

bool Device::isConnected() const {
    return m_controller && m_controller->state() == QLowEnergyController::ConnectedState;
}

bool Device::isInitialized() const {
    return m_initialized;
}

} // namespace dji
