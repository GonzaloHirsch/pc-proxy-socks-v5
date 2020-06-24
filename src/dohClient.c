
#include "dohClient.h"

char *copy_advance(char *ptr, int *length, char *str);

char *request_generate(char *domain, int *length, int qtype)
{

    int request_length = 0;

    int dns_length;
    size_t dns_encoded_length;

    uint8_t *dns_request = generate_dns_req(domain, &dns_length, qtype); //gets the dns request wirh the host name
                                                                         //returns the request and the size of the request

    char *encoded_dns_request = b64_encode((const unsigned char *)dns_request, dns_length, &dns_encoded_length);

    char sendline[BUFFERSIZE_DOH];
    char *ptr = sendline; //pointer used to copy the http request

    char *get_first_part = "GET ";

    ptr = copy_advance(ptr, &request_length, get_first_part);

    char *path = options->doh.path;

    ptr = copy_advance(ptr, &request_length, path);

    char *query = options->doh.query;

    ptr = copy_advance(ptr, &request_length, query);

    //same methodology with encoded dns request

    memcpy(ptr, encoded_dns_request, dns_encoded_length);

    free(encoded_dns_request);
    free(dns_request);

    ptr += dns_encoded_length;
    request_length += dns_encoded_length;

    //now the scheme used

    char *scheme = " HTTP/1.1\r\n";

    ptr = copy_advance(ptr, &request_length, scheme);

    //host we use in this case doh

    char *authority = "Host: ";

    ptr = copy_advance(ptr, &request_length, authority);

    char * host1 = options -> doh.host;

    ptr = copy_advance(ptr, &request_length, host1);

    char *ua = "\r\nUser-Agent: curl/7.54.0\r\n";

    ptr = copy_advance(ptr, &request_length, ua);
    //accept

    char *accept = "accept: application/dns-message\r\n";

    ptr = copy_advance(ptr, &request_length, accept);

    char *connection = "Connection: keep-alive\r\n\r\n";

    ptr = copy_advance(ptr, &request_length, connection);

    (*length) = request_length;

    //now the request is completed

    char *request = malloc(request_length);

    memcpy(request, sendline, request_length);

    return request;
}

char *copy_advance(char *ptr, int *length, char *str)
{
    int size = strlen(str);
    memcpy(ptr, str, size);
    (*length) += size;
    return ptr + size;
}


/*  code from stack overflow */
void parse_to_crlf(uint8_t *response, size_t *size)
{

    int i = 0;
    int j = 0;
    size_t old_size = *size;
    size_t new_size = *size;

    while (i < old_size)
    {

        if (response[i] == '\r')
        {
            i++;
            new_size--;
        }
        else
        {
            response[j] = response[i];
            i++;
            j++;
        }
    }

    *size = new_size;
}
