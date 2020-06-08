#ifndef __HTTP_UTILS_H__
#define __HTTP_UTILS_H__

#define IS_HEADER_NAME_SYMBOL(x) ((x) >= 'a' && (x) <= 'z' || (x) >= 'A' && (x) <= 'Z'\
                             || (x) == '-' || (x) == '\'')

typedef struct http_header{
    char * type;
    char * value;
} http_header;

#endif