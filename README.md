# `libdji` a library to manage DJI Osmo via BLE for Qt

This is actually just a blind port of [github.com/xaionaro-go/djictl (pkg/djictl)](https://github.com/xaionaro-go/djictl/tree/main/pkg/djictl) to Qt.
The last commit reached parity with is: [645a14056614d68613ca47264e3bd26d9ef16c25](https://github.com/xaionaro-go/djictl/commit/645a14056614d68613ca47264e3bd26d9ef16c25).

## Quick demo

A CLI demo is available that drives a real BLE device through pairing, WiFi connection, and streaming start (similar to `cmd/djictl/main.go`).

### Build

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
```

### Run

```bash
./build/tests/dji_demo \
  --wifi-ssid "your-ssid" \
  --wifi-psk "your-psk" \
  --rtmp-url "rtmp://host/app"
```

Optional flags:

* `--filter-device-addr` (default empty): substring match against the BLE device address
* `--timeout` (seconds, default `60`): overall timeout for the flow
