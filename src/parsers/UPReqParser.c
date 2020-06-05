#include "parsers/RecvUPRequestParser.h"

void initializeUPReqParser(struct UPReqParser * p) {

}

enum UPReqState readNextByte(struct UPReqParser *p, const uint8_t b) {
        
    switch(p->state) {
        case UP_REQ_VERSION:

            break;
        case UP_REQ_IDLEN:

            break;
        case UP_REQ_ID:

            break;
        case UP_REQ_PWLEN:

            break;
        case UP_REQ_PW:

            break;
        case UP_REQ_DONE:

            break;
        case UP_ERROR_INV_VERSION:

            break;
        case UP_ERROR_INV_AUTH:

            break;
    }
}
