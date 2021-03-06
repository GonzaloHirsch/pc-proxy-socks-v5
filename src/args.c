#include "args.h"

void free_memory()
{
    free(options);
}

static void
address(char *address, int port, struct sockaddr_in *addr)
{
    int r_4;

    // Try to match with IPv4
    r_4 = inet_pton(AF_INET, address, &addr->sin_addr);

    // IPv4 unsuccessful, try with IPv6
    if (r_4 <= 0)
    {
        printf("Cannot determine address family %s, please try again with a valid address.\n", address);
        fprintf(stderr, "Address resolution error\n");
        free_memory();
        exit(0);
    }
    else
    {
        addr->sin_family = AF_INET;
        addr->sin_port = htons(port);
    }
}

static void
address6(char *address, int port, struct sockaddr_in6 *addr)
{
    // Try to match with IPv4
    int r_6 = inet_pton(AF_INET6, address, &addr->sin6_addr);

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
        addr->sin6_family = AF_INET6;
        addr->sin6_port = htons(port);
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

static int
get_addr_type(char *address)
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
            return AF_INET6;
        }
    }
    else
    {
        return AF_INET;
    }
}

static void
user(char *s, struct users *user)
{
    char *p = strchr(s, ':');
    if (p == NULL)
    {
        fprintf(stderr, "password not found\n");
        free_memory();
        exit(1);
    }
    else
    {
        *p = 0;
        p++;
        user->name = s;
        user->pass = p;
    }
}

static void
version(void)
{
    fprintf(stderr, "socks5v version 0.0\n"
                    "ITBA Protocolos de Comunicación 2020/1 -- Grupo X\n"
                    "AQUI VA LA LICENCIA\n");
}

static void
usage(const char *progname)
{
    fprintf(stderr,
            "Usage: %s [OPTION]...\n"
            "\n"
            "   -h               Imprime la ayuda y termina.\n"
            "   -l <SOCKS addr>  Dirección donde servirá el proxy SOCKS.\n"
            "   -L <conf  addr>  Dirección donde servirá el servicio de management.\n"
            "   -p <SOCKS port>  Puerto entrante conexiones SOCKS.\n"
            "   -P <conf port>   Puerto entrante conexiones configuracion\n"
            "   -u <name>:<pass> Usuario y contraseña de usuario que puede usar el proxy. Hasta 10.\n"
            "   -v               Imprime información sobre la versión versión y termina.\n"
            "\n"
            "   --doh-ip    <ip>    \n"
            "   --doh-port  <port>  XXX\n"
            "   --doh-host  <host>  XXX\n"
            "   --doh-path  <host>  XXX\n"
            "   --doh-query <host>  XXX\n"

            "\n",
            progname);
    free_memory();
    exit(1);
}

// Variable for the options
socks5args options;

void parse_args(const int argc, char *const *argv)
{
    options = malloc(sizeof(*options));
    memset(options, 0, sizeof(*options)); // sobre todo para setear en null los punteros de users

    options->socks_addr = "0.0.0.0";
    options->socks_addr_6 = "::";
    options->socks_port = 1080;
    options->socks_family = AF_UNSPEC;
    memset(&options->socks_addr_info, 0, sizeof(options->socks_addr_info));
    memset(&options->socks_addr_info6, 0, sizeof(options->socks_addr_info6));

    options->mng_addr = "127.0.0.1";
    options->mng_addr_6 = "::1";
    options->mng_port = 8080;
    options->mng_family = AF_UNSPEC;
    memset(&options->mng_addr_info, 0, sizeof(options->mng_addr_info));
    memset(&options->mng_addr_info6, 0, sizeof(options->mng_addr_info6));

    options->disectors_enabled = true;

    options->socks5_buffer_size = 16384;
    options->sctp_buffer_size = 1024;
    options->timeout = 10;

    options->doh.host = "localhost";
    options->doh.ip = "127.0.0.1";
    options->doh.port = 8053;
    options->doh.path = "/getnsrecord";
    options->doh.query = "?dns=";
    options->doh.buffer_size = 1024;
    options->doh.addr_type = get_addr_type(options->doh.ip);

    int c;
    options->n_users = 0;

    while (true)
    {
        int option_index = 0;
        static struct option long_options[] = {
            {"doh-ip", required_argument, 0, 0xD001},
            {"doh-port", required_argument, 0, 0xD002},
            {"doh-host", required_argument, 0, 0xD003},
            {"doh-path", required_argument, 0, 0xD004},
            {"doh-query", required_argument, 0, 0xD005},
            {0, 0, 0, 0}};

        c = getopt_long(argc, argv, "hl:L:Np:P:u:v", long_options, &option_index);
        if (c == -1)
            break;

        switch (c)
        {
        case 'h':
            usage(argv[0]);
            break;
        case 'l':
            options->socks_addr = optarg;
            options->socks_family = get_addr_type(options->socks_addr);
            if (options->socks_family == AF_INET)
            {
                address(options->socks_addr, options->socks_port, &options->socks_addr_info);
            }
            else if (options->socks_family == AF_INET6)
            {
                address6(options->socks_addr, options->socks_port, &options->socks_addr_info6);
            }
            break;
        case 'L':
            options->mng_addr = optarg;
            options->mng_family = get_addr_type(options->mng_addr);
            if (options->mng_family == AF_INET)
            {
                address(options->mng_addr, options->mng_port, &options->mng_addr_info);
            }
            else if (options->mng_family == AF_INET6)
            {
                address6(options->mng_addr, options->mng_port, &options->mng_addr_info6);
            }
            break;
        case 'N':
            options->disectors_enabled = false;
            break;
        case 'p':
            options->socks_port = port(optarg);
            if (options->socks_family == AF_INET6)
            {
                options->socks_addr_info6.sin6_port = htons(options->socks_port);
            }
            else if (options->socks_family == AF_INET)
            {
                options->socks_addr_info.sin_port = htons(options->socks_port);
            }
            break;
        case 'P':
            options->mng_port = port(optarg);
            if (options->mng_family == AF_INET6)
            {
                options->mng_addr_info6.sin6_port = htons(options->mng_port);
            }
            else if (options->mng_family == AF_INET)
            {
                options->mng_addr_info.sin_port = htons(options->mng_port);
            }
            break;
        case 'u':
            if (options->n_users >= MAX_USERS)
            {
                fprintf(stderr, "maximun number of command line users reached: %d.\n", MAX_USERS);
                free_memory();
                exit(1);
            }
            else
            {
                user(optarg, options->users + options->n_users);
                options->n_users++;
            }
            break;
        case 'v':
            version();
            exit(0);
            break;
        case 0xD001:
            options->doh.ip = optarg;
            options->doh.addr_type = get_addr_type(options->doh.ip);
            break;
        case 0xD002:
            options->doh.port = port(optarg);
            break;
        case 0xD003:
            options->doh.host = optarg;
            break;
        case 0xD004:
            options->doh.path = optarg;
            break;
        case 0xD005:
            options->doh.query = optarg;
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
