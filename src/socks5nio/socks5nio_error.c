#include "socks5nio/socks5nio_error.h"

void
send_reply_failure(struct selector_key * key)
{
    struct socks5 *s = ATTACHMENT(key);

    //Reply to be send
    
    // Build the reply
    uint8_t reply_s = 7;
    uint8_t *reply = malloc(reply_s);
    reply[0] = 0x05;            // Version
    reply[1] = s->reply_type != -1 ? s->reply_type : REPLY_RESP_GENERAL_FAILURE;  // Reply field
    reply[2] = 0x00;            //Rsv
    reply[3] = 0x00;
    reply[4] = 0x00;
    reply[5] = 0x00;
    reply[6] = 0x00;

    ssize_t n = send(s->client_fd, reply , reply_s, MSG_DONTWAIT);
    // Metrics
    add_transfered_bytes(n);
    free(reply);
}

void determine_connect_error(int error)
{
    switch (error)
    {
    case EACCES:
        printf("The error is EACCES - %d\n", error);
        break;
    case EPERM:
        printf("The error is EPERM - %d\n", error);
        break;
    case EADDRINUSE:
        printf("The error is EADDRINUSE - %d\n", error);
        break;
    case EADDRNOTAVAIL:
        printf("The error is EADDRNOTAVAIL - %d\n", error);
        break;
    case EAFNOSUPPORT:
        printf("The error is EAFNOSUPPORT - %d\n", error);
        break;
    case EAGAIN:
        printf("The error is EAGAIN - %d\n", error);
        break;
    case EALREADY:
        printf("The error is EALREADY - %d\n", error);
        break;
    case EBADF:
        printf("The error is EBADF - %d\n", error);
        break;
    case ECONNREFUSED:
        printf("The error is ECONNREFUSED - %d\n", error);
        break;
    case EFAULT:
        printf("The error is EFAULT - %d\n", error);
        break;
    case EINPROGRESS:
        printf("The error is EINPROGRESS - %d\n", error);
        break;
    case EINTR:
        printf("The error is EINTR - %d\n", error);
        break;
    case EISCONN:
        printf("The error is EISCONN - %d\n", error);
        break;
    case ENETUNREACH:
        printf("The error is ENETUNREACH - %d\n", error);
        break;
    case ENOTSOCK:
        printf("The error is ENOTSOCK - %d\n", error);
        break;
    case EPROTOTYPE:
        printf("The error is EPROTOTYPE - %d\n", error);
        break;
    case ETIMEDOUT:
        printf("The error is ETIMEDOUT - %d\n", error);
        break;
    case EINVAL:
        printf("The error is EINVAL - %d\n", error);
        break;
    }
}
