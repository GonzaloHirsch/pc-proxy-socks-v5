#include "parsers/ConnectionReqParser.h"
#include "parsers/SOCKS5AddrParser.h"

typedef struct ConnectionReq {
    // public:
    uint8_t ver;
    uint8_t cmd;
    uint8_t rsv;
    Socks5AddrParser socks5AddrParser;
    uint16_t dstPort;
} ConnectionReq;

struct ConnectionReqParser {
    // public:
    ConnectionReq finalMessage;
    ConnectionReqState state;
    // private:
    unsigned int bytesToRead;
};

ConnectionReqParser newConnectionReqParser() {
    ConnectionReqParser crp = malloc(sizeof(struct ConnectionReqParser));
    crp->finalMessage.socks5AddrParser = NULL;
}

enum ConnectionReqState connectionReqReadNextByte(ConnectionReqParser p, const uint8_t b);
enum ConnectionReqState connectionReqConsumeMessage(buffer * b, ConnectionReqParser p, int *errored);
int connectionReqDoneParsing(ConnectionReqParser p, int * errored);
// Free all ConnectionReqParser-Related memory

void freeConnectionReqParser(ConnectionReqParser p);