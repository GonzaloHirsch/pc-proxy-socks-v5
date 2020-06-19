#include "parsers/http_message_parser.h"

#include <string.h>
#include "io_utils/strings.h"

#define CLRF '\n' // I know...

void http_message_parser_init(http_message_parser hmp) {
    // TODO augment struct and manage initialization if necessary
    memset(hmp, 0, sizeof(struct http_message_parser));
    hmp->cursor = hmp->version;
    hmp->body = NULL;
}

enum http_message_state http_message_read_next_byte(http_message_parser p, const uint8_t b) {
    switch(p->state) {
        case HTTP_H:
            if ('H' == b) // TODO check IETF RFCs for case sensitivity
                p->state = HTTP_T1;
            else
                p->state = HTTP_ERR_INV_VERSION;
            break;
        case HTTP_T1:
            if ('T' == b)
                p->state = HTTP_T2;
            else
                p->state = HTTP_ERR_INV_VERSION;
            break;
        case HTTP_T2:
            if ('T' == b)
                p->state = HTTP_P;
            else
                p->state = HTTP_ERR_INV_VERSION;
            break;
        case HTTP_P:
            if ('P' == b)
                p->state = HTTP_SLASH;
            else
                p->state = HTTP_ERR_INV_VERSION;
            break;
        case HTTP_SLASH:
            if ('/' == b) {
                p->state = HTTP_V1;
                p->bytes_to_read = 1;
            }
            else
                p->state = HTTP_ERR_INV_VERSION;
            break;
        case HTTP_V1:
            if (p->bytes_to_read > 0 && b >= '0' && b <= '9') {
                // TODO optimize repeated lines of code
                *p->cursor = b;
                p->cursor++;
                p->bytes_to_read--; 
            }
            else if (p->bytes_to_read == 0 && b == '.') {
                *p->cursor = b;
                p->cursor++;
                p->state = HTTP_V2;
                p->bytes_to_read = 1;
            }
            else 
                p->state = HTTP_ERR_INV_VERSION;
            break;
        case HTTP_V2:
            if (p->bytes_to_read > 0 && b >= '0' && b <= '9') {
                *p->cursor = b;
                p->cursor++;
                p->bytes_to_read--; 
            }
            else if (p->bytes_to_read == 0 && b == ' ') {
                // TODO Validate version
                *p->cursor = '\0';
                p->cursor = p->status;
                p->state = HTTP_STATUS_CODE;
                p->bytes_to_read = 3;
            }
            else 
                p->state = HTTP_ERR_INV_VERSION;
            break;
        case HTTP_STATUS_CODE:
            if (p->bytes_to_read && b >= '0' && b <= '9') {
                *p->cursor = b;
                p->cursor++;
                p->bytes_to_read--;
            }
            else if (p->bytes_to_read == 0 && b == ' ') {
                *p->cursor = '\0';
                p->cursor = p->buff;
                p->state = HTTP_STATUS_MSG;
            }
            else
                p->state = HTTP_ERR_INV_STATUS_CODE;
            break;
        case HTTP_STATUS_MSG:
            // TODO check that status code does not overflow buffer...
            if (b == ' ' || isalpha(b)) {
                *p->cursor = b;
                p->cursor++;
            }
            else if (b == CLRF) {
                *p->cursor = '\0';
                p->state = HTTP_I1;
                p->cursor = p->buff;
            }
            else
                p->state = HTTP_ERR_INV_STATUS_LINE;
            break;
        case HTTP_I1:
            if (b == CLRF) {
                // TODO check content-length
                p->body_len = get_numeric_header_value(p, "content-length");
                p->body = malloc(p->body_len);
                p->cursor = p->body;
                p->state = HTTP_B;
            }
            else if (IS_HEADER_NAME_SYMBOL(b)) {
                p->cursor = p->header_name;
                *p->cursor = b;
                p->cursor++;
                p->state = HTTP_HK;
            }
            else
                p->state = HTTP_ERR_INV_MSG;
            break;
        case HTTP_HK:
            if (IS_HEADER_NAME_SYMBOL(b)) {
                *p->cursor = b;
                p->cursor++;
            }
            else if (b == ':') {
                *p->cursor = '\0';
                // Process Header key
                p->state = HTTP_HV;
                p->cursor = p->header_value;
            }
            else
                p->state = HTTP_ERR_INV_HK;
            break; 
        case HTTP_HV:
            if (b != CLRF) {
                *p->cursor = b;
                p->cursor++;
            }
            else {
                *p->cursor = '\0';
                // Save new header
                p->headers[p->headers_num] = malloc(sizeof(http_header));
                p->headers[p->headers_num]->type = malloc(strlen((const char*) p->header_name)+1);
                p->headers[p->headers_num]->value = malloc(strlen((const char*) p->header_value)+1);
                strcpy(p->headers[p->headers_num]->type, (const char*) p->header_name);
                strcpy(p->headers[p->headers_num]->value, (const char*) p->header_value);
                p->headers_num++;

                p->state = HTTP_I1;
                p->cursor = p->buff;
            }
            break;
        case HTTP_B:
            //if (b == CLRF) {
            //    p->state = HTTP_I2;
            //}
            if(p ->body_len < -1){
                *p->cursor = b;
                p->cursor++;
            }
            else if (p->cursor - p->body < p->body_len) {
                *p->cursor = b;
                p->cursor++;
                if(p->cursor - p->body == p->body_len){
                    //*p->cursor='\0';
                    p->state = HTTP_F;
                }
            }
            else if(p->cursor - p->body == p->body_len){
                    //*p->cursor='\0';
                    p->state = HTTP_F;
                }
            else {
                // ERROR with lengths
                p->state = HTTP_ERR_INV_BODY;
            }
            break;
        case HTTP_I2:
            if (b == CLRF) {
                *p->cursor = '\0'; 
                p->state = HTTP_F;
            }
            else {
                *p->cursor++ = CLRF;
                *p->cursor = b;
                p->cursor++;
            }
            break;
        case HTTP_F:
            // DONE
            printf("Finished\n");
            break;
        case HTTP_ERR_INV_STATUS_LINE:
            
            break;
        case HTTP_ERR_INV_VERSION:
            
            break;
        case HTTP_ERR_INV_STATUS_CODE:
            
            break;
        case HTTP_ERR_INV_HK:
            
            break;
        case HTTP_ERR_INV_HV:
            
            break;
        case HTTP_ERR_INV_BODY:
            
            break;
        case HTTP_ERR_INV_MSG:
            break;
    }
    return p->state;
}

enum http_message_state http_consume_message(buffer * b, http_message_parser p, int *errored) {
    http_message_state st = p->state;
    while(buffer_can_read(b) && !http_done_parsing_message(p, errored)) {
        const uint8_t c = buffer_read(b);
        st = http_message_read_next_byte(p, c);
    }
    return st;
}

int http_done_parsing_message(http_message_parser p, int * errored) {
    if(p->state > HTTP_F){
        *errored = 1;
    }
    return (p->state >= HTTP_F);
}

const char * http_error_string(const http_message_parser p) {
    switch (p->state) {
        case HTTP_ERR_INV_MSG:
            return "Invalid HTTP Message";
        case HTTP_ERR_INV_STATUS_LINE:
            return "Invalid Status Line";
        case HTTP_ERR_INV_VERSION:
            return "Invalid HTTP Version";
        case HTTP_ERR_INV_STATUS_CODE:
            return "Invalid Status Code";
        case HTTP_ERR_INV_HK:
            return "Invalid Header Name";
        case HTTP_ERR_INV_HV:
            return "Invalid Header Value";
        case HTTP_ERR_INV_BODY:
            return "Invalid Header Body";
        default:
            break;
    }
    return "Not an error";
}

int get_numeric_header_value(http_message_parser p, const char * header_name) {
    char * pointer;
    for (int i = 0; i < p->headers_num; i++) {
        if (i_strcmp(p->headers[i]->type, header_name) == 0) {
            return (int) strtol(p->headers[i]->value, &pointer, 10);
        } 
    }

    return -1;
}

http_header ** get_headers(http_message_parser p){
    return p->headers;
}

int get_number_of_headers(http_message_parser p) {
    return p->headers_num;
}

const uint8_t * get_body(http_message_parser p) {
    return p->body;
}

// Free all http_message_parser-Related memory

void free_http_message_parser(http_message_parser p) {
    for (int i = 0; i < p->headers_num; i++) {
        free(p->headers[i]->type);
        free(p->headers[i]->value);
        free(p->headers[i]); 
    }
    if(p->body != NULL){
        free(p->body);
    }
}


http_message_state get_http_state(http_message_parser parser){ return parser -> state;}
