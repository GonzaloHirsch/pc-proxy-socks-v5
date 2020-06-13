#include "socksClient.h"

int main(int argc, char *argv[]) {

  if (argc != 2) // Test for correct number of arguments
    DieWithUserMessage("Parameter(s)",
        "<Request type( ip4 | ip6 | dom )>");

  char *servIP = "127.0.0.1";     // First arg: server IP address (dotted quad)

  // char * reqT = argv[1];
  char * reqT = "ip4";
  // Third arg (optional): server port (numeric).  7 is well-known echo port
  in_port_t servPort =  1080;

  // Create a reliable, stream socket using TCP
  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock < 0)
    DieWithSystemMessage("socket() failed");

  // Construct the server address structure
  struct sockaddr_in servAddr;            // Server address
  memset(&servAddr, 0, sizeof(servAddr)); // Zero out structure
  servAddr.sin_family = AF_INET;          // IPv4 address family
  // Convert address
  int rtnVal = inet_pton(AF_INET, servIP, &servAddr.sin_addr.s_addr);
  if (rtnVal == 0)
    DieWithUserMessage("inet_pton() failed", "invalid address string");
  else if (rtnVal < 0)
    DieWithSystemMessage("inet_pton() failed");
  servAddr.sin_port = htons(servPort);    // Server port

  // Establish the connection to the echo server
  if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0){
    DieWithSystemMessage("connect() failed");
  }

  /* Version 5, one method: no authentication */

  //------------------------HELLO READ------------------------------

  uint8_t data[] = {
        0x05, 0x02, 0x00, 0x03,
    };

  // Send the string to the server
  printf("Sending hello\n");
  ssize_t numBytes = send(sock, data, 4, 0);
  if (numBytes < 0){
    DieWithSystemMessage("send() failed"); 
  }
  else if (numBytes != 4){
    DieWithUserMessage("send()", "sent unexpected number of bytes");
  }

  //--------------------HELLO WRITE-----------------------  

  uint8_t buffer[3]; // I/O buffer
  // Receive up to the buffer size (minus 1 to leave space for
  // a null terminator) bytes from the sender
  numBytes = recv(sock, buffer, 2, 0);
  if (numBytes < 0)
    DieWithSystemMessage("recv() failed");
  else if (numBytes == 0)
    DieWithUserMessage("recv()", "I was terminated by the server");


  buffer[2] = '\0';    // Terminate the string!
  printf("Version %d\n", buffer[0]);
  printf("Method %d\n", buffer[1]);

  //---------------------REQUEST SEND------------------

  uint8_t req4[] = {
    // v  cmd   rsv   typ   ----------ipv4-----     ---port-- 
    0x05, 0x01, 0x00, 0x01, 0x7F, 0x00, 0x00, 0x01, 0x1F, 0x91
  };

  uint8_t req6[22] = {0x05, 0x01, 0x00, 0x04,
   0x04, 0x02, 0x01, 0x01, 0x14, 0x12, 0x11, 0x11, 0x24, 0x22, 0x21, 0x21, 0x34, 0x32, 0x31, 0x31, 
   0x77, 0x77};

  uint8_t reqdom[11] = {0x05, 0x01, 0x00, 0x03,
  0x04, 0x01, 0x02, 0x03, 0x04,  
  0x77, 0x77};



  printf("RQ: %s\n", reqT);

  // Send the string to the server
  printf("Sending request\n");

  if(!strcmp("ip4", reqT)){
    numBytes = send(sock, req4, 10, 0);
  }
  else if(!strcmp("ip6", reqT)){
    numBytes = send(sock, req6, 22, 0);
  }
  else if(!strcmp("dom", reqT)){
    numBytes = send(sock, reqdom, 11, 0);
  }
  else{
    DieWithSystemMessage("invalid req"); 
  }
  
  if (numBytes < 0){
    DieWithSystemMessage("send() failed"); 
  }
  char buff [256];
  recv(sock, buff, 2, 0); //version, status

  if (buff[1] == 0x0)
    printf("Connection request to origin successful!!!\n");
  else {
    printf("Connection request to origin failed\n");
    close(sock);
  }
  
  while(1);
  close(sock);
  exit(0);
}
