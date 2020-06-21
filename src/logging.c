#include "logging.h"

#define ACCESS_CHAR 'A'
#define PASSWORD_CHAR 'P'

/*
    Request logging must contain:
     - Time
     - Username
     - Register Type -> A
     - User IP
     - User PORT
     - Destination IP/NAME
     - Destination PORT
     - Socks Status
*/
void log_request(const int status, const uint8_t *username, const struct sockaddr *clientaddr, const struct sockaddr *originaddr, const uint8_t *fqdn)
{
    // Generating the time buffer
    char time_buff[32] = {0};
    unsigned n = N(time_buff);
    time_t now = 0;
    time(&now);
    strftime(time_buff, n, "%FT%TZ", gmtime(&now)); // tendriamos que usar gmtime_r pero no está disponible en C99

    // Generating the client buffer
    char client_buff[SOCKADDR_TO_HUMAN_MIN] = {0};
    sockaddr_to_human(client_buff, N(client_buff), clientaddr);

    // It means I have a domain
    if (fqdn != NULL && originaddr != NULL)
    {
        // Extracting the port
        in_port_t port = 0;
        switch (originaddr->sa_family)
        {
        case AF_INET:
            port = ((struct sockaddr_in *)originaddr)->sin_port;
            break;
        case AF_INET6:
            port = ((struct sockaddr_in6 *)originaddr)->sin6_port;
            break;
        }
        fprintf(stdout, "%s\t%s\t%c\t%s\t%s\t%d\t%d\n", time_buff, username, ACCESS_CHAR, client_buff, fqdn, ntohs(port), status);
    }
    else
    {
        // Generating the origin buffer
        char origin_buff[SOCKADDR_TO_HUMAN_MIN] = {0};
        sockaddr_to_human(origin_buff, N(origin_buff), originaddr);
        fprintf(stdout, "%s\t%s\t%c\t%s\t%s\t%d\n", time_buff, username, ACCESS_CHAR, client_buff, origin_buff, status);
    }
}

/*
    Request loggin must contain:
     - Time
     - Username
     - Register Type -> P
     - Protocol -> HTTP/POP3
     - Destination IP/NAME
     - Destination PORT
     - Username
     - Password
*/
void log_disector(const uint8_t *owner, struct socks5_origin_info orig_info, const uint8_t *u, const uint8_t *p)
{
    // Generating the time buffer
    char time_buff[32] = {0};
    unsigned n = N(time_buff);
    time_t now = 0;
    time(&now);
    strftime(time_buff, n, "%FT%TZ", gmtime(&now)); // tendriamos que usar gmtime_r pero no está disponible en C99

    char *proto = "UNDEFINED";
    switch (orig_info.protocol_type)
    {
    case PROT_HTTP:
        proto = "HTTP PROTOCOL";
        break;
    case PROT_POP3:
        proto = "POP3 PROTOCOL";
        break;
    default:
        break;
    }

    // It means I have a domain
    if (orig_info.resolve_addr != NULL)
    {
        fprintf(stdout, "%s\t%s\t%c\t%s\t%s\t%s\t%s\t%s\n", time_buff, owner, PASSWORD_CHAR, proto, orig_info.resolve_addr, orig_info.port, u, p);
    }
    else
    {
        // Generating the origin buffer
        char origin_buff[SOCKADDR_TO_HUMAN_MIN] = {0};
        sockaddr_to_human(origin_buff, N(origin_buff), (struct sockaddr *) &orig_info.origin_addr);
        fprintf(stdout, "%s\t%s\t%c\t%s\t%s\t%s\t%s\n", time_buff, owner, PASSWORD_CHAR, proto, origin_buff, u, p);
    }
}