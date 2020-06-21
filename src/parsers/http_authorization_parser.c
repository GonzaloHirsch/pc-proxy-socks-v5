#include "parsers/http_authorization_parser.h"

#define NL '\n'
#define SLASH_R '\r'

void http_auth_init(http_auth_parser p){
    memset(p, 0, sizeof(struct http_auth_parser));
    p -> state = HTTPA_NL;
}



http_auth_state http_auth_consume_byte(http_auth_parser p, uint8_t b){
    switch (p -> state)
    {
    case HTTPA_NL:
        if(b == 'a' || b == 'A'){
            p -> state = HTTPA_A;
        }
        else if(b == SLASH_R){
            p -> state = HTTPA_2BNL;
        }
        else
        {
            p -> state = HTTPA_READ;
        }
        
        break;
    
    case HTTPA_READ:
        if(b == SLASH_R){
            p -> state = HTTPA_BNL;
        }

        break;

    case HTTPA_A:
        if(b == 'u' || b == 'U'){
            p -> state = HTTPA_U;
        }
        else {
            p -> state = HTTPA_READ;
        }
        break;

    case HTTPA_U:
        if(b == 't' || b == 'T'){
            p -> state = HTTPA_T;
        }
                else {
            p -> state = HTTPA_READ;
        }
        break;

    case HTTPA_T:
        if(b == 'h' || b == 'H'){
            p -> state = HTTPA_H;
        }
        else {
            p -> state = HTTPA_READ;
        }
        break;

    case HTTPA_H:
        if(b == 'o' || b == 'O'){
            p -> state = HTTPA_O;
        }
        else {
            p -> state = HTTPA_READ;
        }
        break;
    case HTTPA_O:
        if(b == 'r' || b == 'R'){
            p -> state = HTTPA_R;
        }
        else {
            p -> state = HTTPA_READ;
        }
        break;
    case HTTPA_R:
        if(b == 'i' || b == 'I'){
            p -> state = HTTPA_I;
        }
        else {
            p -> state = HTTPA_READ;
        }
        break;
    case HTTPA_I:
        if(b == 'z' || b == 'Z'){
            p -> state = HTTPA_Z;
        }
        else {
            p -> state = HTTPA_READ;
        }
        break;
    case HTTPA_Z:
        if(b == 'a' || b == 'A'){
            p -> state = HTTPA_A2;
        }
        else {
            p -> state = HTTPA_READ;
        }
        break;

    case HTTPA_A2:
        if(b == 't' || b == 'T'){
            p -> state = HTTPA_T2;
        }
        else {
            p -> state = HTTPA_READ;
        }
        break;
    case HTTPA_T2:
        if(b == 'i' || b == 'I'){
            p -> state = HTTPA_I2;
        }
    else {
            p -> state = HTTPA_READ;
        }
        break;

    case HTTPA_I2:
        if(b == 'o' || b == 'O'){
            p -> state = HTTPA_O2;
        }
                else {
            p -> state = HTTPA_READ;
        }
        break;
    case HTTPA_O2:
        if(b == 'n' || b == 'N'){
            p -> state = HTTPA_N;
        }
        else {
            p -> state = HTTPA_READ;
        }
        break;
    case HTTPA_N:
        if(b == ':'){
            p -> state = HTTPA_S1;

        }
        else if(b == ' '){
            p -> state = HTTPA_N;
        }
        else {
            p -> state = HTTPA_READ;
        }
        break;

    case HTTPA_S1:
        if(b != ' ')
        {
            p -> cursor = p -> buffer;
            p ->in_auth = 1;
            p -> state = HTTPA_G_ENCODING;
        }
        break;

    case HTTPA_G_ENCODING:
        if(b == ' '){ //finished encoding type and has a space as a separator
            *p -> cursor = '\0';
            p ->in_auth = 0;
            p -> encoding = malloc(strlen((const char *) p -> buffer) + 1);
            strcpy((char *) p -> encoding, (char *) p -> buffer);
            p -> state = HTTPA_S2;
        }

        else{
            *p -> cursor = b;
            p -> cursor ++ ;
        }
        break;

    case HTTPA_S2:
    if(b != ' '){
        p ->in_auth = 1;
        p -> cursor = p -> buffer;
        p -> state = HTTPA_G_CONTENT;
    }


    case HTTPA_G_CONTENT:
        if(b == SLASH_R){
            p -> cursor = '\0';
            p -> content = malloc(strlen((const char *) p -> buffer) + 1);
            strcpy((char *) p -> content, (char *) p -> buffer);
            p -> state = HTTPA_F;
        }
        else
        {
            * p-> cursor = b;
            p->cursor++;
        }
        break;

    case HTTPA_BNL:
        if(b == NL){
            p -> in_auth = 0;
            p ->state = HTTPA_NL;
        }
        else if(p ->in_auth) //if there is a slash r inside the authentication
        {
           *p -> cursor = SLASH_R;
           p -> cursor ++;
           *p -> cursor = b;
           p -> cursor ++;
        }
        else
        {
            p -> state = HTTPA_READ;
        }
        
        
    
    default:
        break;
    }


    return p ->state;
}

int http_auth_done_parsing(http_auth_parser p, int * errored){
    if(p -> state >= HTTPA_F || *errored == 1){
        return 1;
    }

    return 0;
}

http_auth_state http_auth_consume_msg(buffer * b, http_auth_parser p, int * errored){

    http_auth_state st = p -> state;
    while(buffer_can_read(b) && !http_auth_done_parsing(p, errored)){
        uint8_t c = buffer_read(b);
        st = http_auth_consume_byte(p, c);
    }

    if(p ->encoding == NULL || p ->content == NULL ){
        *errored =  1;
    }

    return st;
}

void free_http_auth_parser(http_auth_parser p){

    if(p ->encoding != NULL){
        free(p -> encoding);
    }
    if(p -> content != NULL){
        free(p -> content);
    }
}