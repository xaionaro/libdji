#ifndef DJI_CRC_H
#define DJI_CRC_H

#include <QByteArray>
#include <cstdint>

namespace dji {

uint8_t crc8(const QByteArray &data);

uint16_t crc16(const QByteArray &data);

} // namespace dji

#endif
