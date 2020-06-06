#ifndef __HTTP_UTILS_H__
#define __HTTP_UTILS_H__

typedef enum HTTPHeaderType {
    
} HTTPHeaderType;

typedef struct HTTPHeader{
    char * type;
    char * value;
};

#endif