#ifndef __ENCODING_H__
#define __ENCODING_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>


int b64_decode(const char *in, unsigned char *out, size_t outlen);
char *b64_encode(const unsigned char *in, size_t len, size_t *elen);


#endif