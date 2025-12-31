#ifndef CONNECT_FLOW_RUNNER_H
#define CONNECT_FLOW_RUNNER_H

#include <QString>
#include <cstdint>

#include "dji/constants.h"

namespace dji {
class Device;
}

namespace connect_flow {

struct Options {
    QString ssid;
    QString psk;
    QString rtmpUrl;
    dji::Resolution resolution = dji::Resolution::Res1080p;
    uint16_t bitrateKbps = 5000;
    dji::FPS fps = dji::FPS::FPS25;
    int stepTimeoutMs = 10000;
    bool initiateConnection = true;
};

bool runConnectWiFiAndStartStreaming(dji::Device *device, const Options &options,
                                     QString *errorOut = nullptr);

} // namespace connect_flow

#endif
