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
 *         Data structures for controller node's info
 * \author
 *         Fernando Jurado <fdo.jurado@gmail.com>
 */
#ifndef SDN_CTRL_TYPES_H_
#define SDN_CTRL_TYPES_H_

#include "linkaddr.h"

#ifdef SDN_DS_CONF_MAX_NODE_CACHES
#define NODE_CACHES SDN_DS_CONF_MAX_NODE_CACHES + 1
#else
#define NODE_CACHES 10
#endif

/**
 * Notifications sdn-ds-node.h
 */

#ifndef SDN_CONF_DS_NODE_NOTIFICATIONS
#define SDN_DS_NODE_NOTIFICATIONS (NODE_CACHES != 0)
#else
#define SDN_DS_NODE_NOTIFICATIONS SDN_CONF_DS_NODE_NOTIFICATIONS
#endif

/* All network routes */
#ifdef SDN_CONF_NODE_MAX_ROUTES
#define SDN_NODE_MAX_ROUTES SDN_CONF_NODE_MAX_ROUTES
#else
#define SDN_NODE_MAX_ROUTES 10
#endif

/**
 * Notifications sdn-ds-node-route.h
 */
#ifndef SDN_CONF_DS_NODE_ROUTE_NOTIFICATIONS
#define SDN_DS_NODE_ROUTE_NOTIFICATIONS (SDN_NODE_MAX_ROUTES != 0)
#else
#define SDN_DS_NODE_ROUTE_NOTIFICATIONS SDN_CONF_DS_NODE_ROUTE_NOTIFICATIONS
#endif

/**
 * Notifications
 */
#ifndef SDN_CONF_DS_CONFIG_ROUTE_NOTIFICATIONS
#define SDN_DS_CONFIG_ROUTES_NOTIFICATIONS (SDN_NODE_MAX_ROUTES != 0)
#else
#define SDN_DS_CONFIG_ROUTES_NOTIFICATIONS SDN_CONF_DS_NODE_ROUTE_NOTIFICATIONS
#endif

#define NELEMS(x) (sizeof(x) / sizeof((x)[0]))

void sdn_ds_node_id_init(void);

struct nodes_ids
{
    struct nodes_ids *next;
    linkaddr_t addr;
    uint8_t name;
    uint8_t visited;
    /* These are used for clustering */
    uint8_t PCH;
    uint8_t CH;
    struct nodes_ids *parent;
    unsigned long prob;
};

/* array to map ids, save memory */
// extern struct nodes_ids ids[NODE_CACHES];

void sdn_ds_node_id_flush();

struct nodes_ids *nodes_ids_add(const linkaddr_t *addr, const uint8_t name);

struct nodes_ids *nodes_ids_lookup(const linkaddr_t *addr);

struct nodes_ids *nodes_ids_id_lookup(uint8_t id);

int rename_nodes(const uint8_t *);

struct nodes_ids *nodes_ids_max_prob();

int clean_visited(const uint8_t *);

struct nodes_ids *ctrl_node_id(void);

void print_nodes_ids(const uint8_t *);

#endif