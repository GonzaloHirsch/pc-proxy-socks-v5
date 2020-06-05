#include "parsers/UPReqParser.h"

struct UPReqParser {
    // public:      //
    void * data;
    char * uid;
    char * pw;
    // private:     //
    UPReqState state;
    uint8_t uidLen;
    uint8_t pwLen;
    uint8_t bytesToRead;
};

UPReqParser newUPReqParser() {
    UPReqParser uprp = malloc(sizeof(struct UPReqParser));
    uprp->state = UP_REQ_VERSION;
    return uprp;
}

enum UPReqState upReadNextByte(UPReqParser p, const uint8_t b) {   
    switch(p->state) {
        case UP_REQ_VERSION:
            if (0x01 == b)
                p->state = UP_REQ_IDLEN;
            else
                p->state = UP_ERROR_INV_VERSION;
            break;
        case UP_REQ_IDLEN:
            if (b == 0)
                p->state = UP_ERROR_INV_IDLEN;
            else { // (b <= 255)
                p->state = UP_REQ_ID;
                p->uidLen = b;
                // TODO RFC doesn't specify null-terminator. Add one either way
                p->uid = malloc(p->uidLen + 1);
                p->bytesToRead = b;
            }
            break;
        case UP_REQ_ID:
            // if (p->bytesToRead > 0) {
            p->uid[p->uidLen - p->bytesToRead] = b;
            p->bytesToRead--;
            if (p->bytesToRead <= 0) {
                p->uid[p->uidLen+1] = '\0';
                p->state = UP_REQ_PWLEN;            
            }
            break;
        case UP_REQ_PWLEN:
            if (b == 0)
                p->state = UP_ERROR_INV_PWLEN;
            else { // (b <= 255)
                p->state = UP_REQ_PW;
                p->pwLen = b;
                p->pw = malloc(p->pwLen + 1);
                p->bytesToRead = b;
            }
            break;
        case UP_REQ_PW:
            p->pw[p->pwLen - p->bytesToRead] = b;
            p->bytesToRead--;
            if (p->bytesToRead <= 0) {
                p->pw[p->pwLen+1] = '\0';
                p->state = UP_REQ_DONE;            
            }
            break;
        case UP_REQ_DONE:

            break;
        case UP_ERROR_INV_VERSION:
            // break;
        case UP_ERROR_INV_IDLEN:
            // break;
        case UP_ERROR_INV_PWLEN:
            // break;
        case UP_ERROR_INV_AUTH:
            // break;
        default:
            break;
    }
}

enum UPReqState consumeMessage(buffer * b, UPReqParser p, int *errored) {
    enum UPReqState st = p->state;

    while (buffer_can_read(b) && !upDoneParsing(p, errored)) {
        const uint8_t c = buffer_read(b);
        st = upReadNextByte(p, c);
    }
    return st;
}

const char * upErrorString(const UPReqParser p) {
    switch (p->state) {
        case UP_ERROR_INV_VERSION:
            return "Invalid (Unsupported) Version"; 
        case UP_ERROR_INV_IDLEN:
            return "Invalid ID length"; 
        case UP_ERROR_INV_PWLEN:
            return "Invalid Password Length"; 
        case UP_ERROR_INV_AUTH:
            return "Invalid Auth Data";
        }
}

int upDoneParsing(UPReqParser p, int * errored) {
    switch(p->state) {
        case UP_REQ_DONE:
            *errored = 0; 
            return 1;
        case UP_ERROR_INV_VERSION:
            *errored = UP_ERROR_INV_VERSION;
        case UP_ERROR_INV_IDLEN:
            *errored = UP_ERROR_INV_IDLEN;
        case UP_ERROR_INV_PWLEN:
            *errored = UP_ERROR_INV_PWLEN;    
        case UP_ERROR_INV_AUTH:
            *errored = UP_ERROR_INV_AUTH;
            return 1;
        default:
            *errored = 0;
            return 0;
    }
}

void freeUPReqParser(UPReqParser p) {
    free(p->uid);
    free(p->pw);
    free(p);
}