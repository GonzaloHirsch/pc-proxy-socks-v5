#ifndef __SOCKSV5_H__
#define __SOCKSV5_H__

typedef enum AuthOType
{
    NO_AUTH = 0x00,
    USER_PASSWORD = 0x02
} AuthType;

typedef enum AddrType
{
    IPv4 = 0x01,
    DOMAIN_NAME = 0x03,
    IPv6 = 0x04
} AddrType;

typedef enum ClientFdOperation
{
    OP_READ = 0x01,
    OP_WRITE = 0x02
} ClientFdOperation;

/* General State Machine for a server connection */
enum socks_v5state
{
    /**
     * State when receiving the client initial "hello" and processing
     * 
     * Interests:
     *  - OP_READ -> Reading over the client fd
     * 
     * Transitions:
     *  - HELLO_READ -> While the message is not complete
     *  - HELLO_WRITE -> When the message is complete
     *  - ERROR -> In case of an error
     * */
    HELLO_READ,

    /**
     * State when sending the client initial "hello" response
     * 
     * Interests:
     *  - OP_WRITE -> Writing over the client fd
     * 
     * Transitions:
     *  - HELLO_WRITE -> While there are bytes to send
     *  - REQUEST_READ -> When all the bytes have been transfered
     *  - ERROR -> In case of an error
     * */
    HELLO_WRITE,

    /**
     * State when receiving the client request
     * 
     * Interests:
     *  - OP_READ -> Reading over the client fd
     * 
     * Transitions:
     *  - REQUEST_READ -> While the message is not complete
     *  - RESOLVE -> When the message is complete
     *  - ERROR -> In case of an error
     * */
    REQUEST_READ,

    /**
     * State when the client uses a FQDN to connect to another server, the name must be resolved
     * 
     * Interests:
     *  - OP_READ -> Reading over the resolution response fd
     *  - OP_WRITE -> Writing to the DNS server the request
     * 
     * Transitions:
     *  - RESOLVE -> While the name is not resolved
     *  - CONNECTING -> When the name is resolved and the IP address obtained
     *  - ERROR -> In case of an error
     * */
    RESOLVE,

    /**
     * State when connecting to the server the client request is headed to
     * 
     * Interests:
     * 
     * Transitions:
     *  - CONNECTING -> While the server is establishing the connection with the destination of the request
     *  - REPLY -> When the connection is successful and our server can send the request
     *  - ERROR -> In case of an error
     * */
    CONNECTING,

    /**
     * State when sending/receiving the request to/from the destination server
     * 
     * Interests:
     *  - OP_READ -> Reading over the server response
     *  - OP_WRITE -> Writing to server the request
     * 
     * Transitions:
     *  - REPLY -> While the request is being sent and the response is still being copied
     *  - COPY -> When the server response is finished
     *  - ERROR -> In case of an error
     * */
    REPLY,

    /**
     * State when copying the response to the client fd
     * 
     * Interests:
     *  - OP_WRITE -> Writing to client fd
     * 
     * Transitions:
     *  - COPY -> While the server is still copying the response
     *  - DONE -> When the copy process is finished
     *  - ERROR -> In case of an error
     * */
    COPY,

    // estados terminales
    DONE,
    ERROR,
};

#endif