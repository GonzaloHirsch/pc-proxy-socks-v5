#ifndef __AUTH_H__
#define __AUTH_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_USERS 255

bool validate_up(uint8_t * uid, uint8_t * pw, const char * file_name);
bool validate_user_admin(uint8_t * uid, uint8_t * pw);
bool validate_user_proxy(uint8_t * uid, uint8_t * pw);
void list_user_admin(uint8_t ** users, uint8_t * count);

#endif