/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
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
 *         Configuration routes to destinations. This is done to aggregate
 *          all routes config to a node in a single packet.
 * \author
 *         Fernando Jurado <fjurado@student.unimelb.edu.au>
 *
 *
 */
#ifndef SDN_DS_CONFIG_ROUTES_H_
#define SDN_DS_CONFIG_ROUTES_H_

// #include "net/ipv6/uip.h"
#include "sdn-ctrl-types.h"
#include "linkaddr.h"
#include "lib/list.h"

void sdn_ds_config_routes_init(void);

/** \brief An entry in the config routes table */
typedef struct sdn_ds_config_route
{
    struct sdn_ds_config_route *next;
    linkaddr_t scr;
    int8_t ntwk_depth;
    LIST_STRUCT(rt_list);
#if SDN_WITH_TABLE_CHKSUM
    uint16_t recv_chksum;
#endif
    uint8_t acked;
} sdn_ds_config_route_t;

typedef struct sdn_ds_config_route_list
{
    struct sdn_ds_config_route_list *next;
    linkaddr_t via;
    linkaddr_t dest;
#if SDN_WITH_TABLE_CHKSUM
    uint8_t current; // track current configured routes for this round
#endif
} sdn_ds_config_route_list_t;

#if SDN_DS_CONFIG_ROUTES_NOTIFICATIONS
/* Event constants for the uip-ds6 route notification interface. The
   notification interface allows for a user program to be notified via
   a callback when a NBR has been added or removed and when the
   system has added or removed a default route. */
#define SDN_DS_CONFIG_ROUTES_NOTIFICATION_ADD 0
#define SDN_DS_CONFIG_ROUTES_NOTIFICATION_RM 1
#define SDN_DS_CONFIG_ROUTES_NOTIFICATION_CH 2

typedef void (*sdn_ds_config_routes_notification_callback)(int event,
                                                           const sdn_ds_config_route_t *route);
struct sdn_ds_config_routes_notification
{
    struct sdn_ds_config_routes_notification *next;
    sdn_ds_config_routes_notification_callback callback;
};

void sdn_ds_config_routes_notification_add(struct sdn_ds_config_routes_notification *n,
                                           sdn_ds_config_routes_notification_callback c);

void sdn_ds_config_routes_notification_rm(struct sdn_ds_config_routes_notification *n);
/*--------------------------------------------------*/
#endif /* SDN_DS_node_ROUTE_NOTIFICATIONS */

/** \name config routes basic routines */
/** @{ */
sdn_ds_config_route_t *sdn_ds_config_routes_lookup(const linkaddr_t *scr);
sdn_ds_config_route_t *sdn_ds_config_routes_add(const linkaddr_t *scr,
                                                const linkaddr_t *dest,
                                                const linkaddr_t *via);

#if SDN_WITH_TABLE_CHKSUM
sdn_ds_config_route_t *sdn_ds_config_routes_chksum(const linkaddr_t *scr, uint16_t chksum);
#endif

void sdn_ds_config_routes_rm(sdn_ds_config_route_t *);

void sdn_ds_config_routes_rm_dest(sdn_ds_config_route_t *route, const linkaddr_t *dest, const linkaddr_t *via);
// void sdn_ds_config_routes_rm_by_nexthop(const linkaddr_t *nexthop);
sdn_ds_config_route_t *sdn_ds_ntwk_depth_set(const linkaddr_t *, int8_t depth);
sdn_ds_config_route_t *sdn_ds_config_routes_clear_flags(void);
// const linkaddr_t *sdn_ds_config_routes_neighbor(sdn_ds_config_route_t *);
int sdn_ds_config_routes_num_routes(void);
int sdn_ds_config_routes_num_routes_node(const linkaddr_t *addr);
sdn_ds_config_route_t *sdn_ds_config_routes_head(void);
sdn_ds_config_route_t *sdn_ds_config_routes_next(sdn_ds_config_route_t *);
// int sdn_ds_config_routes_is_nexthop(const linkaddr_t *ipaddr);
// int sdn_ds_config_routes_is_neighbor(const linkaddr_t *neighbor);
void sdn_config_routes_flush_all(void);
linkaddr_t *sdn_config_routes_min_rank(void);
linkaddr_t *sdn_config_routes_min_depth(void);

#if SDN_WITH_TABLE_CHKSUM
void sdn_ds_config_routes_clear_current(void);
void sdn_ds_config_routes_delete_current(const linkaddr_t *);
#endif /* SDN_WITH_TABLE_CHKSUM */

#endif