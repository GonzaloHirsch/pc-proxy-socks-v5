#ifndef __DNS_PACKET_H__
#define __DNS_PACKET_H__


//Header Files
#include<stdio.h> //printf
#include<string.h>    //strlen
#include<stdlib.h>    //malloc
#include<sys/socket.h>    //you know what this is for
#include<arpa/inet.h> //inet_addr , inet_ntoa , ntohs etc
#include<netinet/in.h>
#include<unistd.h>    //getpid
#include "socks5nio/socks5nio.h" //socks5 struct


#define MAX_HOST_SIZE 255
#define T_A 1 //Ipv4 address
#define T_NS 2 //Nameserver
#define T_CNAME 5 // canonical name
#define T_SOA 6 /* start of authority zone */
#define T_PTR 12 /* domain name pointer */
#define T_MX 15 //Mail server
#define T_AAAA 28

#define IN 1

//Constant sized fields of query structure
struct QUESTION
{
    unsigned short qtype;
    unsigned short qclass;
};
 
//Constant sized fields of the resource record structure
#pragma pack(push, 1)
struct R_DATA
{
    unsigned short type;
    unsigned short _class;
    unsigned int ttl;
    unsigned short data_len;
};
#pragma pack(pop)
 
//Pointers to resource record contents
struct RES_RECORD
{
    uint8_t *name;
    struct R_DATA *resource;
    uint8_t *rdata;
};
 
//Structure of a Query
typedef struct
{
    uint8_t *name;
    struct QUESTION *ques;
} QUERY;
 


struct DNS_HEADER
{
    unsigned short id; // identification number
 
    uint8_t rd :1; // recursion desired
    uint8_t tc :1; // truncated message
    uint8_t aa :1; // authoritive answer
    uint8_t opcode :4; // purpose of message
    uint8_t qr :1; // query/response flag
 
    uint8_t rcode :4; // response code
    uint8_t cd :1; // checking disabled
    uint8_t ad :1; // authenticated data
    uint8_t z :1; // its z! reserved
    uint8_t ra :1; // recursion available
 
    unsigned short q_count; // number of question entries
    unsigned short ans_count; // number of answer entries
    unsigned short auth_count; // number of authority entries
    unsigned short add_count; // number of resource entries
};



uint8_t * generate_dns_req(char * host, int * final_size, int qtype);

void parse_dns_resp(uint8_t * to_parse, char * domain, struct socks5 * s, int * errored);


#endif