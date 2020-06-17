#ifndef __AUTH_H__
#define __AUTH_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_USER_PASS 500

typedef enum auth_level{
    ADMIN_AUTH_LEVEL = 0,
    USER_AUTH_LEVEL = 1,
} auth_level;

typedef struct user_pass {
    uint8_t * user;
    uint8_t * password;
    uint8_t level;
} user_pass;


int create_up_table();
bool validate_up(uint8_t * uid, uint8_t * pw, uint8_t level);
bool validate_user_admin(uint8_t * uid, uint8_t * pw);
bool validate_user_proxy(uint8_t * uid, uint8_t * pw);

#endif