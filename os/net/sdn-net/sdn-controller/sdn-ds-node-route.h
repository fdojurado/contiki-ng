/*
 * Copyright (c) 2017, RISE SICS.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Data structures for routes of node nodes
 * \author
 *         Fernando Jurado <fdo.jurado@gmail.com>
 */
#ifndef SDN_DS_NODE_ROUTE_H_
#define SDN_DS_NODE_ROUTE_H_

// #include "net/ipv6/uip.h"
#include "sdn-ctrl-types.h"
#include "sys/stimer.h"
#include "lib/list.h"

void sdn_ds_node_route_init(void);

typedef struct sdn_ds_route_node
{
    struct sdn_ds_route_node *next;
    linkaddr_t scr;
    linkaddr_t dest;
    int16_t rssi;
    struct stimer lifetime; //node lifetime
} sdn_ds_route_node_t;

#if SDN_DS_NODE_ROUTE_NOTIFICATIONS
/* Event constants for the uip-ds6 route notification interface. The
   notification interface allows for a user program to be notified via
   a callback when a NBR has been added or removed and when the
   system has added or removed a default route. */
#define SDN_DS_NODE_ROUTE_NOTIFICATION_ADD 0
#define SDN_DS_NODE_ROUTE_NOTIFICATION_RM 1
#define SDN_DS_NODE_ROUTE_NOTIFICATION_CH 2

typedef void (*sdn_ds_node_route_notification_callback)(int event,
                                                        const sdn_ds_route_node_t *link);
struct sdn_ds_route_node_notification
{
    struct sdn_ds_route_node_notification *next;
    sdn_ds_node_route_notification_callback callback;
};

void sdn_ds_node_route_notification_add(struct sdn_ds_route_node_notification *n,
                                        sdn_ds_node_route_notification_callback c);

void sdn_ds_node_route_notification_rm(struct sdn_ds_route_node_notification *n);
/*--------------------------------------------------*/
#endif /* SDN_DS_node_ROUTE_NOTIFICATIONS */

/** \name Node Table basic routines */
/** @{ */
sdn_ds_route_node_t *sdn_ds_node_route_lookup(const linkaddr_t *node, const linkaddr_t *neighbor);
sdn_ds_route_node_t *sdn_ds_node_route_add(const linkaddr_t *node, int16_t rssi,
                                           const linkaddr_t *neighbor);
void sdn_ds_node_route_rm(sdn_ds_route_node_t *route);
// void sdn_ds_node_route_rm_by_nexthop(const linkaddr_t *nexthop);

const linkaddr_t *sdn_ds_node_route_neighbor(sdn_ds_route_node_t *);
int sdn_ds_node_route_num_routes(void);
sdn_ds_route_node_t *sdn_ds_node_route_head(void);
sdn_ds_route_node_t *sdn_ds_node_route_next(sdn_ds_route_node_t *);

void sdn_ds_route_node_print(void);
void sdn_ds_route_node_print_neighbors(const linkaddr_t *);
// int sdn_ds_node_route_is_nexthop(const linkaddr_t *ipaddr);
// int sdn_ds_node_route_is_neighbor(const linkaddr_t *neighbor);
/**
 * The housekeeping function called periodically
 */
void sdn_ds_node_route_periodic(void);
/** @} */

#endif