
#include "dohClient.h"

// code from https://nachtimwald.com/2017/11/18/base64-encode-and-decode-in-c/

const char b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


size_t base64_encoded_size(size_t inlen)
{
	size_t ret;

	ret = inlen;
    
	if (inlen % 3 != 0)
		ret += 3 - (inlen % 3);
        
	ret /= 3;
	ret *= 4;

	return ret;
}

char *base64_encode(const unsigned char *in, size_t len, size_t * elen)
{
	char   *out;

	size_t  i;
	size_t  j;
	size_t  v;

	if (in == NULL || len == 0)
		return NULL;

	*elen = base64_encoded_size(len);
	out  = malloc((*elen)+1);
	out[*elen] = '\0';

	for (i=0, j=0; i<len; i+=3, j+=4) {
		v = in[i];
		v = i+1 < len ? v << 8 | in[i+1] : v << 8;
		v = i+2 < len ? v << 8 | in[i+2] : v << 8;

		out[j]   = b64chars[(v >> 18) & 0x3F];
		out[j+1] = b64chars[(v >> 12) & 0x3F];
		if (i+1 <= len) {
			out[j+2] = b64chars[(v >> 6) & 0x3F];
		} else {
			out[j+2] = '0';
		}
		if (i+2 <= len) {
			out[j+3] = b64chars[v & 0x3F];
		} else {
			out[j+3] = '0';
		}
	}

	return out;
}



// finish code from https://nachtimwald.com/2017/11/18/base64-encode-and-decode-in-c/


// static int mod_table[] = {0, 2, 1};


// char *base64_encode(const unsigned char *data,
//                     size_t input_length,
//                     size_t *output_length) {

//     *output_length = 4 * ((input_length + 2) / 3);

//     char *encoded_data = malloc(*output_length);
//     if (encoded_data == NULL) return NULL;

//     for (int i = 0, j = 0; i < input_length;) {

//         uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
//         uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
//         uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

//         uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

//         encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
//         encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
//         encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
//         encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
//     }

// /*
//     for (int i = 0; i < mod_table[input_length % 3]; i++)
//         encoded_data[*output_length - 1 - i] = '=';

//         */
        

//   return encoded_data;
// }



/* code from stack overflow */

/*
uint8_t * get_host_by_name(char * domain){


    char buffer[BUFFERSIZE_DOH + 1];
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

      ssize_t bytes = recv(sockfd, buffer, BUFFERSIZE_DOH, 0);
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
*/


char * request_generate(char * domain, int *length, int qtype){

int request_length = 0;

int dns_length;
size_t dns_encoded_length;

uint8_t * dns_request = generate_dns_req(domain, &dns_length, qtype);    //gets the dns request wirh the host name 
                                                                //returns the request and the size of the request

char * encoded_dns_request = base64_encode((const unsigned char *)dns_request, dns_length, &dns_encoded_length);




char sendline[BUFFERSIZE_DOH];
char * ptr = sendline; //pointer used to copy the http request

char * get_first_part = "GET /dns-query?dns="; 
int get_first_part_size = strlen(get_first_part);

memcpy(ptr, get_first_part, get_first_part_size);

ptr += get_first_part_size; //add first part size to copy

request_length += get_first_part_size;


//same methodology with encoded dns request

memcpy(ptr, encoded_dns_request, dns_encoded_length);

free(encoded_dns_request);

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


char * accept = "accept: application/dns-message\r\n";

int accept_size = strlen(accept);

memcpy(ptr, accept, accept_size);

ptr += accept_size;
request_length += accept_size;

char * connection = "Connection : keep-alive\r\n\r\n";

int connection_length  = strlen(connection);

memcpy(ptr, connection, connection_length);

ptr += connection_length;
request_length += connection_length;


(*length) = request_length;



//now the request is completed

char * request = malloc(request_length);

memcpy(request, sendline, request_length);



return request;



}

/*
void receive_dns_parse(char * final_buffer, char * domain, int buf_size, struct socks5 * s, int * errored){

    uint8_t * ret;
    parse_to_crlf(final_buffer, &buf_size); //it removes  \r from headers so that parser is consistent

    struct buffer to_parse;


    char data[DATA_MAX_SIZE];
    buffer_init(&to_parse, DATA_MAX_SIZE, (uint8_t *)data);
    int i = 0;

        for (char * b = final_buffer; i<buf_size; b++, i++) {
            buffer_write(&to_parse, *b);
        }

       


    struct http_message_parser http_parser;
    http_message_parser_init(&http_parser);

    http_message_state http_state = http_consume_message(&to_parse, &http_parser, errored);

    char * body;

    if(http_state != HTTP_F){
        perror("Received an unrecognizable http response in doh client");
        exit(EXIT_FAILURE);
    }

    parse_dns_resp(http_parser.body, domain, s, errored);

}
*/

/*  code from stack overflow */
void parse_to_crlf(uint8_t * response, size_t *size){

    int i = 0;
    int j = 0;
    size_t old_size = *size;
    size_t new_size = *size;

    while(i < old_size){

        if(response[i] == '\r'){
            i++;
            new_size--;
        }
        else{
            response[j] = response[i];
            i++;
            j++;
        }
    }

    *size = new_size;
}
