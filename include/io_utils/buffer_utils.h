#ifndef __BUFFER_UTILS_H__
#define __BUFFER_UTILS_H__

#include <stdint.h>
#include "buffer.h"
#include "conversions.h"

bool get_16_bit_integer_from_buffer(buffer *b, uint16_t *n);

bool get_8_bit_integer_from_buffer(buffer *b, uint8_t *n);

#endif