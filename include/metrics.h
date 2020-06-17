#ifndef __METRICS_H__
#define __METRICS_H__

/**
 * Interface to handle metrics in the server.
 * Metrics are volatile, so when the server is shut down they are lost.
 * This interface is to decouple the implementation from the server implementation, so that if required, metrics can be made non-volatile in a file.
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * Metrics struct to hold information
*/
typedef struct metric
{
    uint64_t transfered_bytes;
    uint32_t historic_connections;
    uint32_t current_connections;
    uint8_t metrics_count;
    /*
    uint8_t resolved_addresses;
    uint8_t given_ips;
    uint8_t ipv6_count;
    uint8_t ipv4_count;
    uint8_t log_in_count;
    */
} metric;

/** Metrics pointer redefinition */
typedef metric *metrics;

/**
 * Method to init the metrics pointer
*/
void init_metrics();

void add_transfered_bytes(uint64_t n);
void add_historic_connections(uint32_t n);
void add_current_connections(uint32_t n);
void remove_current_connections(uint32_t n);
/*
void add_resolved_addresses(uint8_t n);
void add_given_ips(uint8_t n);
void add_ipv6(uint8_t n);
void add_ipv4(uint8_t n);
void add_login(uint8_t n);
*/

/**
 * Method to obtain the metrics
*/
metrics get_metrics();

/**
 * Method to destroy the metrics pointer
*/
void destroy_metrics();

#endif
