#include "io_utils/buffer_utils.h"

bool get_16_bit_integer_from_buffer(buffer *b, uint16_t *n)
{
    uint8_t num[2];

    // Reading the request TYPE
    if (buffer_can_read(b))
    {
        num[0] = *b->read;
    }
    else
    {
        return false;
    }

    // Advancing the buffer
    buffer_read_adv(b, 1);

    // Reading the request TYPE
    if (buffer_can_read(b))
    {
        num[1] = *b->read;
    }
    else
    {
        return false;
    }

    // Advancing the buffer
    buffer_read_adv(b, 1);

    *n = ntoh16(num);

    return true;
}

bool get_8_bit_integer_from_buffer(buffer *b, uint8_t *n)
{
    // Reading the request TYPE
    if (buffer_can_read(b))
    {
        *n = *b->read;
    }
    else
    {
        return false;
    }

    // Advancing the buffer
    buffer_read_adv(b, 1);

    return true;
}