#include "metrics.h"

static metrics m;

/**
 * Method to init the metrics pointer
*/
void init_metrics()
{
    m = malloc(sizeof(metrics));
}

void add_transfered_bytes(uint8_t n)
{
    m->transfered_bytes += n;
}

void add_historic_connections(uint8_t n)
{
    m->historic_connections += n;
}

void add_current_connections(uint8_t n)
{
    m->current_connections += n;
}

void remove_current_connections(uint8_t n)
{
    m->current_connections -= n;
}

void add_resolved_addresses(uint8_t n)
{
    m->resolved_addresses += n;
}

void add_given_ips(uint8_t n)
{
    m->given_ips += n;
}

void add_ipv6(uint8_t n)
{
    m->ipv6_count += n;
}

void add_ipv4(uint8_t n)
{
    m->ipv4_count += n;
}

void add_login(uint8_t n)
{
    m->log_in_count += n;
}

char *get_metrics_for_output()
{
    if (m == NULL)
    {
        return NULL;
    }

    // Estimate on the size of the response
    char * result = malloc(320);

    // Format to be used
    char *fmt = "transfered_bytes:%d, historic_connections:%d, current_connections:%d, resolved_addresses:%d, given_ips:%d, ipv6_count:%d, ipv4_count:%d, log_in_count:%d";

    // Printing to the result buffer
    sprintf(result, fmt, m->transfered_bytes, m->historic_connections, m->current_connections, m->resolved_addresses, m->given_ips, m->ipv6_count, m->ipv4_count, m->log_in_count);

    return result;
}

/**
 * Method to destroy the metrics pointer
*/
void destroy_metrics()
{
    free(m);
}