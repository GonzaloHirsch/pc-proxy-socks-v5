enum UPReqState {
    UP_REQ_VERSION,
    UP_REQ_IDLEN,
    UP_REQ_ID,
    UP_REQ_PWLEN,
    UP_REQ_PW,
    UP_REQ_DONE,
    UP_ERROR_INV_VERSION,
    UP_ERROR_INV_IDLEN,
    UP_ERROR_INV_PWLEN,
    UP_ERROR_INV_AUTH
}

typedef struct UPReqParser * UPReqParser;

UPReqParser newUPReqParser();

enum UPReqState readNextByte(UPReqParser p, const uint8_t b);
enum UPReqState consumeMessage(buffer * b, UPReqParser p, bool *errored);

// Free all UPReqParser-Related memory

void freeUPReqParser(UPReqParser p);