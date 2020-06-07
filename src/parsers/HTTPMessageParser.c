#include "parsers/HTTPMessageParser.h"

#define MAX_HEADER_NAME_LEN 40
#define MAX_HEADER_VALUE_LEN 100

#define BLOCK_SIZE 512
#define CLRF '\n' // I know...

struct HTTPMessageParser {
    // public:
    // list of pairs or hash?
    HTTPMessageState state;
    char version[5];
    char status[4];
    // private:
    // char headerName[MAX_HEADER_NAME_LEN+1];
    // char headerValue[MAX_HEADER_VALUE_LEN+1];
    char * cursor;
    int bytesToRead;
    int bodyLen;
    char buff[BLOCK_SIZE];
};

HTTPMessageParser newHTTPMessageParser() {
    // TODO augment struct and manage initialization if necessary
    HTTPMessageParser mp = malloc(sizeof(struct HTTPMessageParser));
    mp->cursor = mp->version;
    return mp;
}

enum HTTPMessageState HTTPMessageReadNextByte(HTTPMessageParser p, const uint8_t b) {
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
                p->bytesToRead = 1;
            }
            else
                p->state = HTTP_ERR_INV_VERSION;
            break;
        case HTTP_V1:
            if (p->bytesToRead > 0 && b >= '0' && b <= '9') {
                // TODO optimize repeated lines of code
                *p->cursor = b;
                p->cursor++;
                p->bytesToRead--; 
            }
            else if (p->bytesToRead == 0 && b == '.') {
                *p->cursor = b;
                p->cursor++;
                p->state = HTTP_V2;
                p->bytesToRead = 1;
            }
            else 
                p->state = HTTP_ERR_INV_VERSION;
            break;
        case HTTP_V2:
            if (p->bytesToRead > 0 && b >= '0' && b <= '9') {
                *p->cursor = b;
                p->cursor++;
                p->bytesToRead--; 
            }
            else if (p->bytesToRead == 0 && b == ' ') {
                // TODO Validate version
                *p->cursor = '\0';
                p->cursor = p->status;
                p->state = HTTP_STATUS_CODE;
                p->bytesToRead = 3;
            }
            else 
                p->state = HTTP_ERR_INV_VERSION;
            break;
        case HTTP_STATUS_CODE:
            if (p->bytesToRead && b >= '0' && b <= '9') {
                *p->cursor = b;
                p->cursor++;
                p->bytesToRead--;
            }
            else if (p->bytesToRead == 0 && b == ' ') {
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
                // Process status code - save?
                p->state = HTTP_I1;
                p->cursor = p->buff;
            }
            else
                p->state = HTTP_ERR_INV_STATUS_LINE;
            break;
        case HTTP_I1:
            if (b == CLRF) {
                p->state = HTTP_B;
            }
            else if (IS_HEADER_NAME_SYMBOL(b)) {
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
                p->cursor = p->buff;
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
                // Process Header Value
                p->state = HTTP_I1;
                p->cursor = p->buff;
            }
            break;
        case HTTP_B:
            if (b == CLRF) {
                p->state = HTTP_I2;
            }
            else {
                *p->cursor = b;
                p->cursor++;
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
    }
}

enum HTTPMessageState HTTPConsumeMessage(buffer * b, HTTPMessageParser p, int *errored) {
    HTTPMessageState st = p->state;
    while(buffer_can_read(b) && !HTTPDoneParsingMessage(p, errored)) {
        const uint8_t c = buffer_read(b);
        st = HTTPMessageReadNextByte(p, c);
    }
    return st;
}

int HTTPDoneParsingMessage(HTTPMessageParser p, int * errored) {
    return (p->state >= HTTP_F);
}

const char * httpErrorString(const HTTPMessageParser p) {
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
    }
}

// Free all HTTPMessageParser-Related memory

void freeHTTPMessageParser(HTTPMessageParser p) {
    free(p);
}
