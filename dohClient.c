#include <netdb.h>
#include <stdio.h>
#include <stdlib.h> // malloc
#include <string.h> // memset
#include <assert.h> // assert
#include <errno.h>
#include <time.h>
#include <unistd.h> // close
#include <pthread.h>
#include <arpa/inet.h>

#define DOH_PORT 80
#define BUFFERSIZE 500
#define MAX_FDQN 256


/*  code from stack overflow */

static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};



static int mod_table[] = {0, 2, 1};


char *base64_encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length) {

    *output_length = 4 * ((input_length + 2) / 3);

    char *encoded_data = malloc(*output_length);
    if (encoded_data == NULL) return NULL;

    for (int i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';

    return encoded_data;
}


/* code from stack overflow */


char * request_generate(char * domain);

struct hostent * get_host_by_name(char * domain){


    char buffer[BUFFERSIZE];
    struct sockaddr_in serv_addr;
    struct hostent * ret;
    char * message;
    int sockfd;

    char * final_buffer;
    char * servIP = "127.0.0.1";

    in_port_t servPort = DOH_PORT;
    memset(&serv_addr, 0, sizeof(serv_addr)); // Zero out structure
    serv_addr.sin_family = AF_INET;          // IPv4 address family
    serv_addr.sin_port = htons(servPort);    // Server port
    // Convert address

    /*
    int rtnVal = inet_pton(AF_INET, servIP, &serv_addr.sin_addr.s_addr);

    if (rtnVal <= 0){
        perror("ERROR converting IP");
        exit(EXIT_FAILURE);
    }
    */
   
   final_buffer = malloc(3);


    int portno  = DOH_PORT;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd <= 0){
        perror("ERROR openning socket");
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        perror("ERROR connecting");
        exit(EXIT_FAILURE);
    }


    int final_buffer_size = 0;

    //char * http_request = request_generate(domain);

   char peer0_0[] = { /* Packet 45 */

0x50, 0x4f, 0x53, 0x54, 0x20, 0x2f, 0x20, 0x48, 

0x54, 0x54, 0x50, 0x2f, 0x31, 0x2e, 0x31, 0x0d, 

0x0a, 0x48, 0x6f, 0x73, 0x74, 0x3a, 0x20, 0x64, 

0x6f, 0x68, 0x0d, 0x0a, 0x55, 0x73, 0x65, 0x72, 

0x2d, 0x41, 0x67, 0x65, 0x6e, 0x74, 0x3a, 0x20, 

0x63, 0x75, 0x72, 0x6c, 0x2d, 0x64, 0x6f, 0x68, 

0x2f, 0x31, 0x2e, 0x30, 0x0d, 0x0a, 0x43, 0x6f, 

0x6e, 0x6e, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 

0x3a, 0x20, 0x55, 0x70, 0x67, 0x72, 0x61, 0x64, 

0x65, 0x2c, 0x20, 0x48, 0x54, 0x54, 0x50, 0x32, 

0x2d, 0x53, 0x65, 0x74, 0x74, 0x69, 0x6e, 0x67, 

0x73, 0x0d, 0x0a, 0x55, 0x70, 0x67, 0x72, 0x61, 

0x64, 0x65, 0x3a, 0x20, 0x68, 0x32, 0x63, 0x0d, 

0x0a, 0x48, 0x54, 0x54, 0x50, 0x32, 0x2d, 0x53, 

0x65, 0x74, 0x74, 0x69, 0x6e, 0x67, 0x73, 0x3a, 

0x20, 0x41, 0x41, 0x4d, 0x41, 0x41, 0x41, 0x42, 

0x6b, 0x41, 0x41, 0x52, 0x41, 0x41, 0x41, 0x41, 

0x41, 0x41, 0x41, 0x49, 0x41, 0x41, 0x41, 0x41, 

0x41, 0x0d, 0x0a, 0x43, 0x6f, 0x6e, 0x74, 0x65, 

0x6e, 0x74, 0x2d, 0x54, 0x79, 0x70, 0x65, 0x3a, 

0x20, 0x61, 0x70, 0x70, 0x6c, 0x69, 0x63, 0x61, 

0x74, 0x69, 0x6f, 0x6e, 0x2f, 0x64, 0x6e, 0x73, 

0x2d, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 0x65, 

0x0d, 0x0a, 0x41, 0x63, 0x63, 0x65, 0x70, 0x74, 

0x3a, 0x20, 0x61, 0x70, 0x70, 0x6c, 0x69, 0x63, 

0x61, 0x74, 0x69, 0x6f, 0x6e, 0x2f, 0x64, 0x6e, 

0x73, 0x2d, 0x6d, 0x65, 0x73, 0x73, 0x61, 0x67, 

0x65, 0x0d, 0x0a, 0x43, 0x6f, 0x6e, 0x74, 0x65, 

0x6e, 0x74, 0x2d, 0x4c, 0x65, 0x6e, 0x67, 0x74, 

0x68, 0x3a, 0x20, 0x32, 0x38, 0x0d, 0x0a, 0x0d, 

0x0a, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 

0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x67, 0x6f, 

0x6f, 0x67, 0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d, 

0x00, 0x00, 0x1c, 0x00, 0x01 };

   // ssize_t http_request_length = strlen(http_request);


    ssize_t numBytes = send(sockfd, peer0_0, 269, 0);

/*

    ssize_t bytes_received = 0;
   do {
        bytes_received = recv(sockfd, buffer, BUFFERSIZE, 0);
        final_buffer = realloc(final_buffer, final_buffer_size + bytes_received + 1);
        memcpy(final_buffer + final_buffer_size, buffer, bytes_received);
        final_buffer_size += bytes_received;
        final_buffer[final_buffer_size + 1] = 0x00;
    } while(bytes_received != 0);
    */

    ssize_t bytes = recv(sockfd, buffer, BUFFERSIZE, 0);

    buffer[bytes] = 0;


    printf("%s", buffer);



    return ret;


}


char * request_generate(char * domain){

    char * answer;

    return answer;

}





char * parse_domain(char * domain){
    int current = 0;
    int parse_current = 0;

    static char parsed[MAX_FDQN];
    while(domain[current] != 0) {
        char letter = domain[current];
        if(letter != '.'){
            parsed[parse_current++] = letter;
        }
        current++;
    }

    parsed[parse_current] = 0;

    return parsed;

}


int main(){


    char * example = "www.google.com";

    get_host_by_name(example);

    return 1;
}