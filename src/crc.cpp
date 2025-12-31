#include "dji/crc.h"
#include <QtGlobal>

namespace dji {

static uint8_t reverseBits(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

static const uint8_t CRC8_POLY_REV = 0x8C;
static const uint8_t CRC8_INIT = 0x77;

uint8_t crc8(const QByteArray &data) {
    uint8_t crc = CRC8_INIT;
    for (char c : data) {
        uint8_t byte = static_cast<uint8_t>(c);
        crc ^= byte;
        for (int i = 0; i < 8; ++i) {
            if (crc & 1) {
                crc = (crc >> 1) ^ CRC8_POLY_REV;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

static const uint16_t CRC16_POLY_REV = 0x8408;
static const uint16_t CRC16_INIT = 0x3692;

uint16_t crc16(const QByteArray &data) {
    uint16_t crc = CRC16_INIT;
    for (char c : data) {
        uint8_t byte = static_cast<uint8_t>(c);
        crc ^= byte;
        for (int i = 0; i < 8; ++i) {
            if (crc & 1) {
                crc = (crc >> 1) ^ CRC16_POLY_REV;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

} // namespace dji
