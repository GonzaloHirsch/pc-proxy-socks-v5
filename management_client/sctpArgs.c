#include "sctpArgs.h"

void free_memory()
{
    if (args->mng_addr_info != NULL)
    {
        free(args->mng_addr_info);
    }
    free(args);
}

static void
address(char *address, int port, int protocol, int *family, struct addrinfo *addrinfo)
{
    struct in_addr inaddr;
    struct in6_addr in6addr;
    int r_4, r_6;

    // Try to match with IPv4
    r_4 = inet_pton(AF_INET, address, &inaddr);

    // IPv4 unsuccessful, try with IPv6
    if (r_4 <= 0)
    {
        // Try to match with IPv4
        r_6 = inet_pton(AF_INET6, address, &in6addr);

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
            ((struct sockaddr_in6 *)addrinfo)->sin6_family = AF_INET6;
            ((struct sockaddr_in6 *)addrinfo)->sin6_port = htons(port);
            ((struct sockaddr_in6 *)addrinfo)->sin6_addr = in6addr;
        }
    }
    else
    {
        *family = AF_INET;
        ((struct sockaddr_in *)addrinfo)->sin_family = AF_INET;
        ((struct sockaddr_in *)addrinfo)->sin_port = htons(port);
        ((struct sockaddr_in *)addrinfo)->sin_addr = inaddr;
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
        free_memory();
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
    free_memory();
    exit(1);
}

// Variable for the options
sctpClientArgs args;

void parse_args(const int argc, char **argv)
{
    args = malloc(sizeof(*args));
    memset(args, 0, sizeof(*args));

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
            args->mng_addr_info = malloc(sizeof(struct addrinfo *));
            address(args->mng_addr, args->mng_port, IPPROTO_SCTP, &args->mng_family, args->mng_addr_info);
            break;
        case 'P':
        printf("CHANGING PIRT\n");
            args->mng_port = port(optarg);
            if (args->mng_addr_info != NULL)
            {
                if (args->mng_family == AF_INET6)
                {
                    ((struct sockaddr_in6 *)args->mng_addr_info)->sin6_port = htons(args->mng_port);
                }
                else if (args->mng_family == AF_INET)
                {
                    ((struct sockaddr_in *)args->mng_addr_info)->sin_port = htons(args->mng_port);
                }
            }
            break;
        case 'v':
            version();
            free_memory();
            exit(0);
            break;
        default:
            fprintf(stderr, "unknown argument %d.\n", c);
            free_memory();
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
        free_memory();
        exit(1);
    }
}
