#include "args.h"

static unsigned short
port(const char *s) {
     char *end     = 0;
     const long sl = strtol(s, &end, 10);

     if (end == s|| '\0' != *end
        || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
        || sl < 0 || sl > USHRT_MAX) {
         fprintf(stderr, "port should in in the range of 1-65536: %s\n", s);
         exit(1);
         return 1;
     }
     return (unsigned short)sl;
}

static void
user(char *s, struct users *user) {
    char *p = strchr(s, ':');
    if(p == NULL) {
        fprintf(stderr, "password not found\n");
        exit(1);
    } else {
        *p = 0;
        p++;
        user->name = s;
        user->pass = p;
    }

}

static void
version(void) {
    fprintf(stderr, "socks5v version 0.0\n"
                    "ITBA Protocolos de Comunicación 2020/1 -- Grupo X\n"
                    "AQUI VA LA LICENCIA\n");
}

static void
usage(const char *progname) {
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
    exit(1);
}

// Variable for the options
socks5args options;

void 
parse_args(const int argc, char * const*argv) {
    options = malloc(sizeof(*options));
    memset(options, 0, sizeof(*options)); // sobre todo para setear en null los punteros de users

    options->socks_addr = "0.0.0.0";
    options->socks_port = 1080;

    options->mng_addr   = "127.0.0.1";
    options->mng_port   = 8080;

    options->disectors_enabled = true;

    options->socks5_buffer_size = 4096;
    options->sctp_buffer_size = 1024;
    options->timeout = 10;

    options->doh.host = "localhost";
    options->doh.ip   = "127.0.0.1";
    options->doh.port = 8053;
    options->doh.path = "/getnsrecord";
    options->doh.query = "?dns=";
    options->doh.buffer_size = 1024;

    int c;
    int nusers = 0;

    while (true) {
        int option_index = 0;
        static struct option long_options[] = {
            { "doh-ip",    required_argument, 0, 0xD001 },
            { "doh-port",  required_argument, 0, 0xD002 },
            { "doh-host",  required_argument, 0, 0xD003 },
            { "doh-path",  required_argument, 0, 0xD004 },
            { "doh-query", required_argument, 0, 0xD005 },
            { 0,           0,                 0, 0 }
        };

        c = getopt_long(argc, argv, "hl:L:Np:P:u:v", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 'h':
                usage(argv[0]);
                break;
            case 'l':
                options->socks_addr = optarg;
                break;
            case 'L':
                options->mng_addr = optarg;
                break;
            case 'N':
                options->disectors_enabled = false;
                break;
            case 'p':
                options->socks_port = port(optarg);
                break;
            case 'P':
                options->mng_port   = port(optarg);
                break;
            case 'u':
                if(nusers >= MAX_USERS) {
                    fprintf(stderr, "maximun number of command line users reached: %d.\n", MAX_USERS);
                    exit(1);
                } else {
                    user(optarg, options->users + nusers);
                    nusers++;
                }
                break;
            case 'v':
                version();
                exit(0);
                break;
            case 0xD001:
                options->doh.ip = optarg;
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
                exit(1);
        }

    }
    if (optind < argc) {
        fprintf(stderr, "argument not accepted: ");
        while (optind < argc) {
            fprintf(stderr, "%s ", argv[optind++]);
        }
        fprintf(stderr, "\n");
        exit(1);
    }
}
