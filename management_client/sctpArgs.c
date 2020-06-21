#include "sctpArgs.h"

static void
address(char *address, int port, int protocol, int * family, struct addrinfo **addrinfo)
{
    unsigned char buff[sizeof(struct in6_addr)];
    int r_4, r_6;

    // Try to match with IPv4
    r_4 = inet_pton(AF_INET, address, buff);

    // IPv4 unsuccessful, try with IPv6
    if (r_4 <= 0)
    {
        // Try to match with IPv4
        r_6 = inet_pton(AF_INET6, address, buff);

        // IPv6 error, exit
        if (r_6 <= 0)
        {
            printf("Cannot determine address family %s, please try again with a valid address.\n", address);
            fprintf(stderr, "Address resolution error\n");
            free_memory();
            exit(0);
        }
        else
        {
            *family = AF_INET6;
        }
    }
    else
    {
        *family = AF_INET;
    }

    struct addrinfo hints = {
        .ai_family = *family,
        .ai_socktype = SOCK_STREAM,
        .ai_flags = AI_PASSIVE,
        .ai_protocol = protocol,
        .ai_canonname = NULL,
        .ai_addr = NULL,
        .ai_next = NULL,
    };

    char port_buff[7];
    sprintf(port_buff, "%hu", port);
    if (0 != getaddrinfo(address, port_buff, &hints, addrinfo))
    {
        printf("Cannot resolve %s, please try again with a valid address.\n", address);
        fprintf(stderr, "Domain name resolution error\n");
        free_memory();
        exit(0);
    }
}

static unsigned short
port(const char *s)
{
    char *end = 0;
    const long sl = strtol(s, &end, 10);

    if (end == s || '\0' != *end || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno) || sl < 0 || sl > USHRT_MAX)
    {
        fprintf(stderr, "port should in in the range of 1-65536: %s\n", s);
        exit(1);
        return 1;
    }
    return (unsigned short)sl;
}

static void
version(void)
{
    fprintf(stderr, "sctp client version 0.0\n"
                    "ITBA Protocolos de Comunicación 2020/1 -- Grupo 2\n"
                    "AQUI VA LA LICENCIA\n");
}

static void
usage(const char *progname)
{
    fprintf(stderr,
            "Usage: %s [OPTION]...\n"
            "\n"
            "   -h               Imprime la ayuda y termina.\n"
            "   -L <conf  addr>  Dirección donde servirá el servicio de management.\n"
            "   -P <conf port>   Puerto entrante conexiones configuracion\n"
            "   -v               Imprime información sobre la versión versión y termina.\n"
            "\n",
            progname);
    exit(1);
}

void parse_args(const int argc, char **argv, struct sctpClientArgs *args)
{
    memset(args, 0, sizeof(*args)); // sobre todo para setear en null los punteros de users

    args->mng_addr = "127.0.0.1";
    args->mng_port = 8080;

    int c;

    while (true)
    {
        c = getopt(argc, argv, "hL:P:v");
        if (c == -1)
            break;

        switch (c)
        {
        case 'h':
            usage(argv[0]);
            break;
        case 'L':
            args->mng_addr = optarg;
            break;
        case 'P':
            args->mng_port = port(optarg);
            break;
        case 'v':
            version();
            exit(0);
            break;
        default:
            fprintf(stderr, "unknown argument %d.\n", c);
            exit(1);
        }
    }
    if (optind < argc)
    {
        fprintf(stderr, "argument not accepted: ");
        while (optind < argc)
        {
            fprintf(stderr, "%s ", argv[optind++]);
        }
        fprintf(stderr, "\n");
        exit(1);
    }
}
