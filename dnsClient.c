
#include "dnsClient.h"



int conectDns(char * url){

    struct dns_packet * packet;
    struct sockaddr_in sin;
    int sin_len = sizeof (sin);
    int sin_length = sizeof(sin);
    int buf_len;

    int sock;
    char buffer[MAX_BUFF];
    char * servIP = "8.8.8.8";

    sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	memset ((char *) &sin, 0, sizeof (sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons (DNS_PORT);
    inet_aton (servIP , &sin.sin_addr);

     //connect(sock, &sin, sizeof(servIP));

    packet = calloc (1, sizeof (struct dns_packet));
	packet->header.id = 2048;

    memcpy (buffer, &packet->header, 16);
	buf_len = 12;
	memcpy (buffer + buf_len, url, strlen(url)*8 + 8);
	buf_len += sizeof (url);

	sendto (sock, buffer, buf_len, 0, (struct sockaddr *) &sin, sin_len);
	recv (sock, buffer, 255, 0);

	printf ("%s\n", buffer);

    close(sock);

	return 0;





}



int main(int argc, char ** argv){


}