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
#ifndef SDN_DS_EDGE_H_
#define SDN_DS_EDGE_H_

// #include "net/ipv6/uip.h"
#include "net/nbr-table.h"
#include "sys/stimer.h"
#include "lib/list.h"

void sdn_ds_edge_init(void);

/** \brief An entry in the routing table */
typedef struct sdn_ds_edge
{
    struct sdn_ds_edge *next;
    linkaddr_t scr,
        dest;
} sdn_ds_edge_t;

/**
 * Notifications
 */
#ifndef SDN_CONF_DS_EDGE_NOTIFICATIONS
#define SDN_DS_EDGE_NOTIFICATIONS (SDN_EDGE_MAX != 0)
#else
#define SDN_DS_EDGE_NOTIFICATIONS SDN_CONF_DS_EDGE_NOTIFICATIONS
#endif

#if SDN_DS_EDGE_NOTIFICATIONS
/* Event constants for the uip-ds6 route notification interface. The
   notification interface allows for a user program to be notified via
   a callback when a NBR has been added or removed and when the
   system has added or removed a default route. */
#define SDN_DS_EDGE_NOTIFICATION_ADD 0
#define SDN_DS_EDGE_NOTIFICATION_RM 1
#define SDN_DS_EDGE_NOTIFICATION_CH 2

typedef void (*sdn_ds_edge_notification_callback)(int event,
                                                  int num_routes);
struct sdn_ds_edge_notification
{
    struct sdn_ds_edge_notification *next;
    sdn_ds_edge_notification_callback callback;
};

void sdn_ds_edge_notification_add(struct sdn_ds_edge_notification *n,
                                  sdn_ds_edge_notification_callback c);

void sdn_ds_edge_notification_rm(struct sdn_ds_edge_notification *n);
/*--------------------------------------------------*/
#endif /* SDN_DS_node_ROUTE_NOTIFICATIONS */

/** \name Node Table basic routines */
/** @{ */
sdn_ds_edge_t *sdn_ds_edge_lookup(const linkaddr_t *node, const linkaddr_t *neighbor);
sdn_ds_edge_t *sdn_ds_edge_add(const linkaddr_t *node, const linkaddr_t *neighbor);

void sdn_ds_edge_rm(sdn_ds_edge_t *);
void sdn_ds_edge_flush();

void sdn_ds_edge_remove_addr(const linkaddr_t *);

int sdn_ds_edge_num_edges(void);

sdn_ds_edge_t *sdn_ds_edge_head(void);
sdn_ds_edge_t *sdn_ds_edge_tail(void);
sdn_ds_edge_t *sdn_ds_edge_next(sdn_ds_edge_t *);

/**
 * The housekeeping function called periodically
 */
void sdn_ds_edge_periodic(void);
/** @} */

#endif