#ifndef DJI_CONSTANTS_H
#define DJI_CONSTANTS_H

#include <QByteArray>
#include <cstdint>

namespace dji {

enum class SubsystemID : uint16_t {
    Status = 0x00,
    Configurer = 0x0201,
    Pairer = 0x0207,
    Streamer = 0x0208,
    PrePairer = 0x0402,
    OneMorePairer = 0x0288,
};

enum class MessageID : uint16_t {
    PairingStarted = 0x7911,
    SetPairingPIN = 0x72AA,
    PairingStage1 = 0x0400,
    PairingStage2 = 0x74AA,
    PrepareToLiveStreamStage1 = 0xFEAB,
    PrepareToLiveStreamStage2 = 0xFFAB,
    StartScanningWiFi = 0x8EBB,
    ConnectToWiFi = 0x98BB,
    ConfigureStreaming = 0xB3BB,
    StartStreaming = 0xB4BB,
    StopStreaming = 0xB5BB,
};

enum class MessageType : uint32_t {
    Configure = 0x40028E,

    MaybeStatus = 0x000405,
    MaybeKeepAlive = 0x000427,

    PairingStage2 = 0x400032,
    PairingStarted = 0x000280,
    SetPairingPIN = 0x400745,
    PairingStatus = 0xC00745,
    PairingPINApproved = 0x400746,
    PairingStage1 = 0xC00746,
    ConnectToWiFi = 0x400747,
    ConnectToWiFiResult = 0xC00747,
    StartScanningWiFi = 0x4007AB,
    StartScanningWiFiResult = 0xC007AB,
    WiFiScanReport = 0x4007AC,

    StartStopStreaming = 0x40028E,
    StartStopStreamingResult = 0x80028E,
    PrepareToLiveStream = 0x4002E1,
    PrepareToLiveStreamResult = 0xC002E1,
    ConfigureStreaming = 0x400878,
    StreamingStatus = 0x000D02,

    Unknown0 = 0x400081,
    Unknown1 = 0x0000F1,
    Unknown2 = 0x0002DC,
    Unknown3 = 0x00041C,
    Unknown4 = 0x000438,
    Unknown5 = 0x000745,
};

enum class DeviceType : uint8_t {
    Undefined = 0,
    Unknown,
    OsmoAction3,
    OsmoAction4,
    OsmoAction5Pro,
    OsmoPocket3,
};

inline uint8_t deviceTypeToByte(DeviceType t) {
    switch (t) {
    case DeviceType::OsmoAction5Pro:
        return 0x2E;
    default:
        return 0x2A;
    }
}

inline uint8_t deviceTypeToStabilizationByte(DeviceType t) {
    switch (t) {
    case DeviceType::OsmoAction5Pro:
        return 0x1A;
    default:
        return 0x08;
    }
}

inline DeviceType identifyDeviceType(const QByteArray &manufacturerData) {

    if (manufacturerData.size() < 2)
        return DeviceType::Undefined;

    if (manufacturerData[0] == 0x12 && manufacturerData[1] == 0x00)
        return DeviceType::OsmoAction3;
    if (manufacturerData[0] == 0x14 && manufacturerData[1] == 0x00)
        return DeviceType::OsmoAction4;
    if (manufacturerData[0] == 0x15 && manufacturerData[1] == 0x00)
        return DeviceType::OsmoAction5Pro;
    if (manufacturerData[0] == 0x20 && manufacturerData[1] == 0x00)
        return DeviceType::OsmoPocket3;

    return DeviceType::Unknown;
}

enum class Resolution : uint8_t {
    Undefined = 0,
    Res480p = 0x47,
    Res720p = 0x04,
    Res1080p = 0x0A,
};

enum class FPS : uint8_t {
    Undefined = 0,
    FPS25 = 0x02,
    FPS30 = 0x03,
};

enum class ImageStabilization : uint8_t {
    Undefined = 0,
    Off = 0x00,
    RockSteady = 0x01,
    HorizonSteady = 0x02,
    RockSteadyPlus = 0x03,
    HorizonBalancing = 0x04,
};

} // namespace dji

#endif
