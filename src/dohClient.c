#include "dnsPacket.h"
#include "dohClient.h"
#include "parsers/http_message_parser.h"

#define DOH_PORT 80
#define BUFFERSIZE 1024
#define MAX_FDQN 256
#define DATA_MAX_SIZE 65536

#define REQ_MAX_SIZE 65536


/*  code from stack overflow */

void parse_to_crlf(char * response, int *size);

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

/*
    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';

        */
        

    return encoded_data;
}


/* code from stack overflow */


uint8_t * get_host_by_name(char * domain){


    char buffer[BUFFERSIZE + 1];
    struct sockaddr_in serv_addr;
    uint8_t * ret;
    char * message;
    int sockfd;

    char * final_buffer = NULL;
    char * servIP = "127.0.0.1";

    in_port_t servPort = DOH_PORT;
    memset(&serv_addr, 0, sizeof(serv_addr)); // Zero out structure
    serv_addr.sin_family = AF_INET;          // IPv4 address family
    serv_addr.sin_port = htons(servPort);    // Server port



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



    char * http_request = request_generate(domain, &final_buffer_size);

    send(sockfd, http_request,final_buffer_size, 0);

    //free("http_request");

    ssize_t bytes = 0;
    int buf_size = 0;

  do{

      ssize_t bytes = recv(sockfd, buffer, BUFFERSIZE, 0);
      if(bytes < 0){
          perror("DOH recv failed");
          exit(EXIT_FAILURE);
      }
      else
      {
          final_buffer = realloc(final_buffer, final_buffer_size + bytes);
          if(final_buffer == NULL){
              perror("Error in doh final buffer realloc");
              exit(EXIT_FAILURE);
          }
          buf_size += bytes;
          memcpy(final_buffer, buffer, bytes);
      }
      
  }while (bytes > 0);
  



parse_to_crlf(final_buffer, &buf_size);

struct buffer to_parse;


char data[DATA_MAX_SIZE];
int errored;
buffer_init(&to_parse, DATA_MAX_SIZE, (uint8_t *)data);
int i = 0;

    for (char * b = final_buffer; i<buf_size; b++, i++) {
        buffer_write(&to_parse, *b);
    }

    buffer_write(&to_parse, '\n');
    buffer_write(&to_parse, '\n');


struct http_message_parser http_parser;
http_message_parser_init(&http_parser);

http_message_state http_state = http_consume_message(&to_parse, &http_parser, &errored);

char * body;

if(http_state != HTTP_F){
    perror("Received an unrecognizable http response in doh client");
    exit(EXIT_FAILURE);
}



ret = parse_dns_resp(http_parser.body, domain);


    return ret;


}


char * request_generate(char * domain, int *length){

int request_length = 0;

int dns_length;
size_t dns_encoded_length;

uint8_t * dns_request = generate_dns_req(domain, &dns_length);    //gets the dns request wirh the host name 
                                                                //returns the request and the size of the request

char * encoded_dns_request = base64_encode((char *)dns_request, dns_length,&dns_encoded_length);


char sendline[BUFFERSIZE];
char * ptr = sendline; //pointer used to copy the http request

char * get_first_part = "GET /dns-query?dns="; 
int get_first_part_size = strlen(get_first_part);

memcpy(ptr, get_first_part, get_first_part_size);

ptr += get_first_part_size; //add first part size to copy

request_length += get_first_part_size;


//same methodology with encoded dns request

memcpy(ptr, encoded_dns_request, dns_encoded_length);

ptr += dns_encoded_length;
request_length += dns_encoded_length;


//now the scheme used

char * scheme = " HTTP/1.1\r\n";
int scheme_size = strlen(scheme);

memcpy(ptr, scheme, scheme_size);

ptr += scheme_size;
request_length += scheme_size;


//host we use in this case doh

char * authority = "Host: doh\r\nUser-Agent: curl/7.54.0\r\n";
int authority_size = strlen(authority);

memcpy(ptr, authority, authority_size);

ptr += authority_size;
request_length += authority_size;

//accept


char * accept = "accept: application/dns-message\r\n\r\n";

int accept_size = strlen(accept);

memcpy(ptr, accept, accept_size);

ptr += accept_size;
request_length += accept_size;

(*length) = request_length;


//now the request is completed

char * request = malloc(request_length);

memcpy(request, sendline, request_length);



return request;



}





void parse_to_crlf(char * response, int *size){

    int i = 0;
    int j = 0;
    int old_size = *size;
    int new_size = *size;

    while(i < old_size){

        if(response[i] == '\r'){
            i++;
            new_size --;
        }
        else{
            response[j] = response[i];
            i++;
            j++;
        }
    }

    *size = new_size;


}

/*
int main(){


    char * example = "www.facebook.com";

    int size;
    size_t size2;




    uint8_t * ret = get_host_by_name(example);

    printf("%s\n", ret);


    return 0;
}
*/