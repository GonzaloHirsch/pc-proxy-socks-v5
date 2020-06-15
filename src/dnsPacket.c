
#include "dnsPacket.h"

void host_dns_format(uint8_t * dns, char * host);
uint8_t * ReadName(unsigned char* reader,uint8_t * buffer,int* count);


uint8_t * generate_dns_req(char * host, int * final_size){

    int length = 0;

    uint8_t buf[65536] ,*qname, *qname2;


    struct DNS_HEADER *dns = NULL;
    struct QUESTION *qinfo = NULL;
     struct QUESTION *qinfo2 = NULL;


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
    dns->q_count = htons(1); 
    dns->ans_count = 0;
    dns->auth_count = 0;
    dns->add_count = 0;

    length += sizeof(struct DNS_HEADER); //size of the struct


    qname = (uint8_t*) (buf + sizeof(struct DNS_HEADER));

    

    host_dns_format(qname , host);

    length += strlen((const char *)qname) + 1; //size of the host plus the '.' translated


    qinfo =(struct QUESTION*)&buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)]; //fill it
 
    qinfo->qtype = htons(T_A); //type of the query , A , MX , CNAME , NS etc
    qinfo->qclass = htons(IN); //internet



    length += sizeof(struct QUESTION);
    /*

    qname2 = (uint8_t *) (buf + sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1) + sizeof(struct QUESTION));

    host_dns_format(qname2 , host);

    length += strlen((const char *)qname2) + 1; //size of the host plus the '.' translated


    qinfo2 =(struct QUESTION*)&buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1) + sizeof(struct QUESTION) + (strlen((const char*)qname2) + 1)]; //fill it
 
    qinfo2->qtype = htons(T_AAAA); //type of the query , A , MX , CNAME , NS etc
    qinfo2->qclass = htons(IN); //internet


    length += sizeof(struct QUESTION);
    */

    uint8_t * ret = malloc(length);

    memcpy(ret,buf, length);


    *final_size = length;


    return ret;


}

//if it is www.google.com it converts to 3www6google3com

 void host_dns_format(uint8_t * dns, char * host){ //we need this later to parse

    int lock = 0 , i;
    
    char * host_name = malloc(strlen((const char *)host) + 2);
    strcpy((char *)host_name, host);
    strcat((char*)host_name,".");
     
    for(i = 0 ; i < strlen((char *)host_name) ; i++) 
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


uint8_t * parse_dns_resp(uint8_t * to_parse, char * domain){
    int stop, i, j;
    struct DNS_HEADER *dns = NULL;
    char * reader, *reader2, * ret;

    struct sockaddr_in a;

   

    dns = (struct DNS_HEADER*) to_parse;
    reader = &to_parse[sizeof(struct DNS_HEADER) + ((strlen((const char*) domain)+ 2) + sizeof(struct QUESTION)) ]; //domain + 2 becuse host_dns_format adds a '.' and also a '\0' also * 2 because there are 2 queries
    struct RES_RECORD answers[20];

    for(i=0;i<ntohs(dns->ans_count);i++)
    {
        answers[i].name=ReadName(reader,to_parse,&stop);
        reader = reader + stop;
 
        answers[i].resource = (struct R_DATA*)(reader);
        reader = reader + sizeof(struct R_DATA);
 
        if(ntohs(answers[i].resource->type) == 1) //if its an ipv4 address
        {
            answers[i].rdata = (uint8_t *)malloc(ntohs(answers[i].resource->data_len));
 
            for(j=0 ; j<ntohs(answers[i].resource->data_len) ; j++)
            {
                answers[i].rdata[j]=reader[j];
            }
 
            answers[i].rdata[ntohs(answers[i].resource->data_len)] = '\0';
 
            reader = reader + ntohs(answers[i].resource->data_len);
        }
        else
        {
            answers[i].rdata = ReadName(reader,to_parse,&stop);
            reader = reader + stop;
        }
    }
    //print answers
    for(i=0 ; i < ntohs(dns->ans_count) ; i++)
    {
 
        if( ntohs(answers[i].resource->type) == T_A) //IPv4 address
        {
            long *p;
            p= (long*) answers[i].rdata;
            a.sin_addr.s_addr=(*p); //working without ntohl
            ret = inet_ntoa(a.sin_addr);
        }   
         if( ntohs(answers[i].resource->type) == T_AAAA) //IPv4 address
        {
            char dest[500];
            long *p;
            p=(long*)answers[i].rdata;
            a.sin_addr.s_addr=(*p); //working without ntohl
            ret = (char *)inet_ntop(AF_INET6, &(a.sin_addr),dest, INET6_ADDRSTRLEN);
        }   
        
    }
    return ret;
}


uint8_t * ReadName(unsigned char* reader,uint8_t * buffer,int* count)
{
    uint8_t *name;
    unsigned int p=0,jumped=0,offset;
    int i , j;
 
    *count = 1;
    name = (uint8_t *)malloc(256);
 
    name[0]='\0';
 
    //read the names in 3www6google3com format
    while(*reader!=0)
    {
        if(*reader>=192) //this is in case of compressed 
        {
            offset = (*reader)*256 + *(reader+1) - 49152; //49152 = 11000000 00000000 ;)
            reader = buffer + offset - 1;
            jumped = 1; //we have jumped to another location so counting wont go up!
        }
        else
        {
            name[p++]=*reader;
        }
 
        reader = reader+1;
 
        if(jumped==0)
        {
            *count = *count + 1; //if we havent jumped to another location then we can count up
        }
    }
 
    name[p]='\0'; //string complete
    if(jumped==1)
    {
        *count = *count + 1; //number of steps we actually moved forward in the packet
    }
 
    //now convert 3www6google3com0 to www.google.com
    for(i=0;i<(int)strlen((const char*)name);i++) 
    {
        p=name[i];
        for(j=0;j<(int)p;j++) 
        {
            name[i]=name[i+1];
            i=i+1;
        }
        name[i]='.';
    }
    name[i-1]='\0'; //remove the last dot
    return name;
}