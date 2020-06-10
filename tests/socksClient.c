#include "socksClient.h"

int main(int argc, char *argv[]) {

  if (argc < 2 || argc > 3) // Test for correct number of arguments
    DieWithUserMessage("Parameter(s)",
        "<Server Address> <Echo Word> [<Server Port>]");

  char *servIP = argv[1];     // First arg: server IP address (dotted quad)

  // Third arg (optional): server port (numeric).  7 is well-known echo port
  in_port_t servPort = (argc == 3) ? atoi(argv[2]) : 1080;

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
    

  uint8_t buffer[3]; // I/O buffer
  // Receive up to the buffer size (minus 1 to leave space for
  // a null terminator) bytes from the sender
  numBytes = recv(sock, buffer, 2, 0);
  if (numBytes < 0)
    DieWithSystemMessage("recv() failed");
  else if (numBytes == 0)
    DieWithUserMessage("recv()", "connection closed prematurely");


  buffer[2] = '\0';    // Terminate the string!
  printf("Version %d\n", buffer[0]);
  printf("Method %d\n", buffer[1]);


  close(sock);
  exit(0);
}
