#ifndef __UTILS_CONVERSIONS_H__
#define __UTILS_CONVERSIONS_H__

#include <stdint.h>

void hton64(uint8_t *b, uint64_t n);

uint64_t ntoh64(uint8_t const *b);

void hton32(uint8_t *b, uint32_t n);

uint32_t ntoh32(uint8_t const *b);

void hton16(uint8_t *b, uint16_t n);

uint16_t ntoh16(uint8_t const *b);

#endif