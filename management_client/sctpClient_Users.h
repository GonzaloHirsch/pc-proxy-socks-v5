#ifndef __SCTPCLIENT_USERS_H__
#define __SCTPCLIENT_USERS_H__

#include "sctpClient.h"

int try_log_in(uint8_t *username, uint8_t *password);

void handle_create_user();

void handle_list_users();

#endif