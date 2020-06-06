#include "parsers/HTTPMessageParser.h"

#define MAX_HEADER_NAME_LEN 40
#define MAX_HEADER_VALUE_LEN 50

struct HTTPMessageParser {
    // public:
    // list of pairs or hash?
    
    // private:
    char headerName[MAX_HEADER_NAME_LEN+1];
    char headerValue[MAX_HEADER_VALUE_LEN+1];

};
