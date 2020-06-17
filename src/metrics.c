#include "metrics.h"

static metrics m;

/**
 * Method to init the metrics pointer
*/
void init_metrics()
{
    m = malloc(sizeof(metrics));
}

void add_transfered_bytes(uint64_t n)
{
    m->transfered_bytes += n;
}

void add_historic_connections(uint32_t n)
{
    m->historic_connections += n;
}

void add_current_connections(uint32_t n)
{
    m->current_connections += n;
}

void remove_current_connections(uint32_t n)
{
    m->current_connections -= n;
}

/*
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
*/

static metrics get_metrics(){
    return m;
}

/**
 * Method to destroy the metrics pointer
*/
void destroy_metrics()
{
    free(m);
}