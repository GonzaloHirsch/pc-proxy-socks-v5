#ifndef __POP3_PARSER_H___ 
#define __POP3_PARSER_H___ 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>

#include "io_utils/buffer.h"

#define MAX_LINE 512


typedef enum pop3_state{
    POP3_NL,
    POP3_READING,
    POP3_U_U,
    POP3_U_S,
    POP3_U_E,
    POP3_U_R,
    POP3_G_USER,
    POP3_P_P,
    POP3_P_A,
    POP3_P_S1,
    POP3_P_S2,
    POP3_G_PASS,
    POP3_F,
    POP3_ERR_NO_U

} pop3_state;

struct pop3_parser
{

    uint8_t * cursor;
    uint8_t * start;
    int h_user;
    uint8_t * user;
    uint8_t * pass;
    pop3_state state;
    uint8_t buff[MAX_LINE];
};

typedef struct pop3_parser * pop3_parser;

void pop3_parser_init(pop3_parser p);
pop3_state pop3_consume_msg(buffer * b, pop3_parser p, int * errored);
void free_pop3_parser(pop3_parser p);
int pop3_done_parsing(pop3_parser p,int * errored);



#endif



