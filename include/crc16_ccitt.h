#ifndef CRC16_CCITT_H_
#define CRC16_CCITT_H_

#include <stddef.h>
#include <stdint.h>

// CRC-16/CCITT-FALSE: poly=0x1021 init=0xFFFF refin=false refout=false xorout=0x0000
uint16_t crc16_ccitt_false(const uint8_t *data, size_t len);

#endif  // CRC16_CCITT_H_
