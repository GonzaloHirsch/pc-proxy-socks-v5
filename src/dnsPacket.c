
#include "dnsPacket.h"

void host_dns_format(char * dns, char * host);


char * generate_dns_req(char * host, int * final_size){

    int length = 0;

    char buf[65536] ,*qname;


    struct DNS_HEADER *dns = NULL;
    struct QUESTION *qinfo = NULL;


    dns = (struct DNS_HEADER *)&buf; //overlaps header into the buffer


    dns->id = 0; //number to identify dns request (unsigned short) htons(getpid())
    dns->qr = 0; //This is a query
    dns->opcode = 0; //This is a standard query
    dns->aa = 0; //Not Authoritative
    dns->tc = 0; //This message is not truncated
    dns->rd = 1; //Recursion Desired
    dns->ra = 0; //Recursion not available! hey we dont have it (lol)
    dns->z = 0;
    dns->ad = 0;
    dns->cd = 0;
    dns->rcode = 0;
    dns->q_count = htons(1); //we have only 1 question
    dns->ans_count = 0;
    dns->auth_count = 0;
    dns->add_count = 0;

    length += sizeof(struct DNS_HEADER); //size of the struct


    qname = (char*) (buf + sizeof(struct DNS_HEADER));

    

    host_dns_format(qname , host);

    length += strlen(qname) + 1; //size of the host plus the '.' translated


    qinfo =(struct QUESTION*)&buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)]; //fill it
 
    qinfo->qtype = htons(T_A); //type of the query , A , MX , CNAME , NS etc
    qinfo->qclass = htons(1); //internet 
    

    length += sizeof(struct QUESTION);

    char * ret = malloc(length);

    memcpy(ret,buf, length);


    *final_size = length;


    return ret;


}

//if it is www.google.com it converts to 3www6google3com

 void host_dns_format(char * dns, char * host){

    int lock = 0 , i;

    char * host_name = malloc(strlen(host) + 2);
    strcpy(host_name, host);
    strcat((char*)host_name,".");
     
    for(i = 0 ; i < strlen((char*)host_name) ; i++) 
    {
        if(host_name[i]=='.') 
        {
            *dns++ = i-lock;
            for(;lock<i;lock++) 
            {
                *dns++=host_name[lock];
            }
            lock++; 
        }
    }
    *dns++='\0';
}


