#include "io_utils/conversions.h"

void hton64(uint8_t *b, uint64_t n)
{

    b[0] = n >> 56 & 0xFF;
    b[1] = n >> 48 & 0xFF;
    b[2] = n >> 40 & 0xFF;
    b[3] = n >> 32 & 0xFF;
    b[4] = n >> 24 & 0xFF;
    b[5] = n >> 16 & 0xFF;
    b[6] = n >> 8 & 0xFF;
    b[7] = n >> 0 & 0xFF;
}

uint64_t ntoh64(uint8_t const *b)
{
    return (uint64_t)b[0] << 56 |
           (uint64_t)b[1] << 48 |
           (uint64_t)b[2] << 40 |
           (uint64_t)b[3] << 32 |
           (uint64_t)b[4] << 24 |
           (uint64_t)b[5] << 16 |
           (uint64_t)b[6] << 8 |
           (uint64_t)b[7] << 0;
}

void hton32(uint8_t *b, uint32_t n)
{
    b[0] = n >> 24 & 0xFF;
    b[1] = n >> 16 & 0xFF;
    b[2] = n >> 8 & 0xFF;
    b[3] = n >> 0 & 0xFF;
}

uint32_t ntoh32(uint8_t const *b)
{
    return (uint32_t)b[0] << 24 |
           (uint32_t)b[1] << 16 |
           (uint32_t)b[2] << 8 |
           (uint32_t)b[3] << 0;
}
