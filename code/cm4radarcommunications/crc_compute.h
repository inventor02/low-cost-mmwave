#ifndef CRC_COMPUTE_H_
#define CRC_COMPUTE_H_

#include <stdint.h>

uint64_t computeCRC(uint8_t *p, uint32_t len, uint8_t width);

#endif