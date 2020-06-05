enum UPReqState {
    UP_REQ_VERSION,
    UP_REQ_IDLEN,
    UP_REQ_ID,
    UP_REQ_PWLEN,
    UP_REQ_PW,
    UP_REQ_DONE,
    UP_ERROR_INV_VERSION,
    UP_ERROR_INV_AUTH
}

struct UPReqParser {
    void * data;
    
    UPReqState state;
    uint8_t bytesToRead;
};

typedef struct UPReqParser * UPReqParser;

void initializeUPReqParser(struct UPReqParser * p);
enum UPReqState readNextByte(struct UPReqParser * p, const uint8_t b);
enum UPReqState consumeMessage(buffer * b, struct UPReqParser *p, bool *errored);
