
#include "dnsPacket.h"

void host_dns_format(uint8_t * dns, char * host);
uint8_t * ReadName(unsigned char* reader,uint8_t * buffer,int* count);


uint8_t * generate_dns_req(char * host, int * final_size, int qtype){

    int length = 0;

    uint8_t buf[65536] ,*qname;


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
    dns->q_count = htons(1); 
    dns->ans_count = 0;
    dns->auth_count = 0;
    dns->add_count = 0;

    length += sizeof(struct DNS_HEADER); //size of the struct


    qname = (uint8_t*) (buf + sizeof(struct DNS_HEADER));

    

    host_dns_format(qname , host);

    length += strlen((const char *)qname) + 1; //size of the host plus the '.' translated


    qinfo =(struct QUESTION*)&buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)]; //fill it
 
    qinfo->qtype = htons(qtype); //type of the query , A , MX , CNAME , NS etc
    qinfo->qclass = htons(IN); //internet



    length += sizeof(struct QUESTION);

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
    free(host_name);
    *dns++='\0';
}


void parse_dns_resp(uint8_t * to_parse, char * domain, struct socks5 * s, int * errored){
    int stop, i, j;
    struct DNS_HEADER *dns = NULL;
    unsigned char * reader;   

    dns = (struct DNS_HEADER*) to_parse;
    reader = &to_parse[sizeof(struct DNS_HEADER) + ((strlen((const char*) domain)+ 2) + sizeof(struct QUESTION)) ]; //domain + 2 becuse host_dns_format adds a '.' and also a '\0' also * 2 because there are 2 queries
    struct RES_RECORD answers[20];

    if(ntohs(dns-> ans_count) == 0){
        *errored = 1;
    }

    for(i=0;i<ntohs(dns->ans_count);i++)
    {
        answers[i].name=ReadName((unsigned char *) reader,to_parse,&stop); //this is not needed for the project but it can be information that can be logged in the future
        reader = reader + stop;
 
        answers[i].resource = (struct R_DATA*)(reader);
        reader = reader + sizeof(struct R_DATA);
 
        if(ntohs(answers[i].resource->type) == T_A || ntohs(answers[i].resource->type) == T_AAAA) //if its an ipv4 address
        {
            answers[i].rdata = (uint8_t *)malloc(1+ntohs(answers[i].resource->data_len));
 
            for(j=0 ; j<ntohs(answers[i].resource->data_len) ; j++)
            {
                answers[i].rdata[j]=reader[j];
            }
 
            answers[i].rdata[ntohs(answers[i].resource->data_len)] = '\0';
 
            reader = reader + ntohs(answers[i].resource->data_len);
        }
        else{
            reader = reader + ntohs(answers[i].resource->data_len); //if something that is not ipv4 or ipv6 comes then just forward the pointer
        }
         
    }


    for(i=0 ; i < ntohs(dns->ans_count) ; i++) //it could all be done in one iteration but to better understanding dns processing and our structure of socks 5 was divided
    {
 
        if( ntohs(answers[i].resource->type) == T_A) //IPv4 address
        {
            uint8_t *p;
            p=  answers[i].rdata;
            memcpy(s->origin_info.ipv4_addrs[s->origin_info.ipv4_c++], p, IP_V4_ADDR_SIZE);
            free(answers[i].rdata);
            free(answers[i].name);

        }   
        else if( ntohs(answers[i].resource->type) == T_AAAA) //IPv6 address
        {
            uint8_t *p;
            p= answers[i].rdata;
            memcpy(s->origin_info.ipv6_addrs[s->origin_info.ipv6_c++], p, IP_V6_ADDR_SIZE);
            free(answers[i].rdata);
            free(answers[i].name);

        }   
        else
        {
            free(answers[i].name); 
        }
        
\
        
    }
}


uint8_t * ReadName(unsigned char* reader,uint8_t * buffer,int* count)
{
    uint8_t * name;
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
            memcpy(name + p, reader, 1);
            p++;
            //name[p++]=*reader;
        }
 
        reader = reader+1;
 
        if(jumped==0)
        {
            *count = *count + 1; //if we havent jumped to another location then we can count up
        }
    }

    char terminate = '\0';
    char * dot = ".";
 
    memcpy(name + p, &terminate, 1);
    //name[p]='\0'; //string complete
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
        memcpy(name + i, dot, 1);
        //name[i]='.';
    }
    memcpy(name + i - 1, &terminate, 1);//remove the last dot
    //name[i-1]='\0'; 
    return name;
}