#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <sys/types.h>
#include <stdbool.h>
#define MAX_BUFF 2048
#define DNS_PORT 53


struct dns_header
{
	u_int16_t id; /* a 16 bit identifier assigned by the client */
	u_int16_t qr:1;
	u_int16_t opcode:4;
	u_int16_t aa:1;
	u_int16_t tc:1;
	u_int16_t rd:1;
	u_int16_t ra:1;
	u_int16_t z:3;
	u_int16_t rcode:4;
	u_int16_t qdcount;
	u_int16_t ancount;
	u_int16_t nscount;
	u_int16_t arcount;
};

struct dns_packet
{
	struct dns_header header;
//	struct dns_question question;
	char *data;
	u_int16_t data_size;
};

struct dns_response_packet
{
	char *name;
	u_int16_t type;
	u_int16_t classes;
	u_int32_t ttl;
	u_int16_t rdlength;
	char *rdata;
};

struct dns_question
{
	char *qname;
	u_int16_t qtype;
	u_int16_t qclass;
};