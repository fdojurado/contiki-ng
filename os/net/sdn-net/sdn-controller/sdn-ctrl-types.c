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
 *         Data structures for routes of network nodes
 * \author
 *         Fernando Jurado <fdo.jurado@gmail.com>
 */
#include "sdn-ctrl-types.h"
#include "sdn-net/sdn.h"
#include "sdn-ds-node.h"

#include "lib/list.h"
#include "lib/memb.h"

/* Log configuration */
#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

LIST(nodeidlist);
MEMB(nodeidmemb, struct nodes_ids, NODE_CACHES);

static int num_node_ids = 0;
// static struct nodes_ids ids[NODE_CACHES];

/*---------------------------------------------------------------------------*/
void sdn_ds_node_id_init(void)
{
    memb_init(&nodeidmemb);
    list_init(nodeidlist);
}
/*---------------------------------------------------------------------------*/
struct nodes_ids *
sdn_ds_node_id_head(void)
{
    return list_head(nodeidlist);
}
/*---------------------------------------------------------------------------*/
struct nodes_ids *
sdn_ds_node_id_next(struct nodes_ids *r)
{
    return list_item_next(r);
}
/*---------------------------------------------------------------------------*/
struct nodes_ids *nodes_ids_lookup(const linkaddr_t *addr)
{
    struct nodes_ids *r;

    if (addr == NULL)
    {
        return NULL;
    }

    // longestmatch = 0;
    for (r = list_head(nodeidlist);
         r != NULL;
         r = list_item_next(r))
    {
        if (linkaddr_cmp(&r->addr, addr))
            return r;
    }

    PRINTF("nodes_ids_lookup not found\n");
    return NULL;
}
/*---------------------------------------------------------------------------*/
struct nodes_ids *nodes_ids_max_prob()
{
    unsigned long max = 0L;
    struct nodes_ids *node;
    struct nodes_ids *r;

    node = NULL;

    for (r = list_head(nodeidlist);
         r != NULL;
         r = list_item_next(r))
    {
        if (r->prob > max && !r->CH)
        {
            max = r->prob;
            node = r;
        }
    }

    return node;
}
/*---------------------------------------------------------------------------*/
struct nodes_ids *nodes_ids_id_lookup(uint8_t id)
{
    struct nodes_ids *node;
    struct nodes_ids *r;

    node = NULL;

    for (r = list_head(nodeidlist);
         r != NULL;
         r = list_item_next(r))
    {
        if (r->name == id)
            node = r;
    }

    return node;
}
/*---------------------------------------------------------------------------*/
struct nodes_ids *nodes_ids_add(const linkaddr_t *addr, const uint8_t name)
{
    if (addr == NULL)
    {
        return NULL;
    }

    struct nodes_ids *r;

    r = nodes_ids_lookup(addr);

    /* First make sure that we don't add edge twice */
    if (r == NULL)
    {
        /* Allocate memory for new node */
        r = memb_alloc(&nodeidmemb);
        if (r == NULL)
        {
            PRINTF("Couldn't allocate more node ids (%d.%d)\n",
                   addr->u8[0], addr->u8[1]);
            return NULL;
        }
        linkaddr_copy(&r->addr, addr);
        r->name = name;
        list_push(nodeidlist, r);
        num_node_ids++;
    }

    r->visited = 0;
    r->PCH = 0;
    r->CH = 0;
    r->prob = 0;
    r->parent = NULL;

    return r;
}
/*---------------------------------------------------------------------------*/
static void sdn_ds_node_id_rm(struct nodes_ids *r)
{
    if (r == NULL)
        return;

    list_remove(nodeidlist, r);
    memb_free(&nodeidmemb, r);
    num_node_ids--;

    return;
}
/*---------------------------------------------------------------------------*/
void sdn_ds_node_id_flush()
{
    struct nodes_ids *r;
    r = list_head(nodeidlist);
    while (r != NULL)
    {
        sdn_ds_node_id_rm(r);
        r = list_head(nodeidlist);
    }

    return;
}
/*---------------------------------------------------------------------------*/
int rename_nodes(const uint8_t *num_vx)
{
    // if (*num_vx > NELEMS(ids))
    //     return 0;

    sdn_ds_node_t *node;
    uint8_t i;
    sdn_ds_node_id_flush();
    /* Rename the nodes to process the links */
    nodes_ids_add(&ctrl_addr, 0);
    // ids[0].addr = &ctrl_addr;
    // ids[0].name = 0;
    // ids[0].visited = 0;
    i = 1;
    for (node = sdn_ds_node_head();
         node != NULL;
         node = sdn_ds_node_next(node))
    {
        if (node->energy > 0)
        {
            nodes_ids_add(&node->addr, i);
            i++;
        }
    }

    print_nodes_ids(num_vx);

    return 1;
}
/*---------------------------------------------------------------------------*/
int clean_visited(const uint8_t *num_vx)
{
    struct nodes_ids *r;

    r = nodes_ids_lookup(&ctrl_addr);

    r->visited = 0;
    r->parent = NULL;

    for (r = list_head(nodeidlist);
         r != NULL;
         r = list_item_next(r))
    {
        r->visited = 0;
        r->parent = NULL;
    }

    print_nodes_ids(num_vx);

    return 1;
}
/*---------------------------------------------------------------------------*/
struct nodes_ids *ctrl_node_id(void)
{
    return nodes_ids_lookup(&ctrl_addr);
}
/*---------------------------------------------------------------------------*/
void print_nodes_ids(const uint8_t *num_vx)

{

    struct nodes_ids *r;

    for (r = list_head(nodeidlist);
         r != NULL;
         r = list_item_next(r))
    {
        PRINTF("node %d (%d.%d) visited %d PCH %d CH %d prob %lu\n",
               r->name,
               r->addr.u8[0], r->addr.u8[1],
               r->visited,
               r->PCH,
               r->CH,
               r->prob);
    }
}
