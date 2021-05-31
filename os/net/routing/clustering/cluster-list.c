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
#include "cluster-list.h"

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

#ifdef SDN_CONF_cluster_MAX
#define SDN_cluster_MAX SDN_CONF_cluster_MAX
#else
#define SDN_cluster_MAX 20
#endif

/* Each cluster is repressented by a uip_ds6_route_t structure and
   memory for each cluster is allocated from the cluster_list_memb memory
   block. These routes are maintained on the cluster_list. */
LIST(cluster_list);
MEMB(cluster_list_memb, cluster_list_t, SDN_cluster_MAX);

static int num_clusters = 0;
// static void rm_routelist_callback(nbr_table_item_t *ptr);

static void cluster_list_print(void);

/*---------------------------------------------------------------------------*/
void cluster_list_init(void)
{
    memb_init(&cluster_list_memb);
    list_init(cluster_list);
}
/*---------------------------------------------------------------------------*/
cluster_list_t *
cluster_list_head(void)
{
    return list_head(cluster_list);
}
/*---------------------------------------------------------------------------*/
cluster_list_t *
cluster_list_tail(void)
{
    return list_tail(cluster_list);
}
/*---------------------------------------------------------------------------*/
cluster_list_t *
cluster_list_next(cluster_list_t *r)
{
    return list_item_next(r);
}
/*---------------------------------------------------------------------------*/
int cluster_list_num_clusters(void)
{
    return num_clusters;
}
/*---------------------------------------------------------------------------*/
cluster_list_t *
cluster_list_lookup(const linkaddr_t *addr)
{
    cluster_list_t *r;
    cluster_list_t *found_cluster;

    PRINTF("Looking up for cluster: %d.%d\n",
           addr->u8[0], addr->u8[1]);

    if (addr == NULL)
    {
        return NULL;
    }

    found_cluster = NULL;
    // longestmatch = 0;
    for (r = cluster_list_head();
         r != NULL;
         r = cluster_list_next(r))
        if (linkaddr_cmp(&r->addr, addr))
            found_cluster = r;

    if (found_cluster != NULL)
    {
        PRINTF("Cluster found: %d.%d\n",
               addr->u8[0], addr->u8[1]);
    }
    else
    {
        PRINTF("No cluster found\n");
    }

    if (found_cluster != NULL && found_cluster != list_head(cluster_list))
    {
        /* If we found a cluster, we put it at the start of the routeslist
       list. The list is ordered by how recently we looked them up:
       the least recently used cluster will be at the end of the
       list - for fast lookups (assuming multiple packets to the same node). */

        list_remove(cluster_list, found_cluster);
        list_push(cluster_list, found_cluster);
    }

    return found_cluster;
}
/*---------------------------------------------------------------------------*/
cluster_list_t *
cluster_list_add(const linkaddr_t *addr, const linkaddr_t *ch_addr, uint8_t CH)
{
    if (addr == NULL)
    {
        return NULL;
    }
    cluster_list_t *cluster;

    cluster = cluster_list_lookup(addr);

    if (cluster != NULL)
        return cluster;

    /* Allocate memory for new node */
    cluster = memb_alloc(&cluster_list_memb);
    if (cluster == NULL)
    {
        PRINTF("Couldn't allocate more clusters %d.%d\n",
               addr->u8[0], addr->u8[1]);
        return NULL;
    }
    linkaddr_copy(&cluster->addr, addr);
    linkaddr_copy(&cluster->ch_addr, ch_addr);
    list_add(cluster_list, cluster);
    num_clusters++;
    cluster->CH = CH;

    cluster_list_print();
    return cluster;
}
/*---------------------------------------------------------------------------*/
void cluster_list_flush()
{
    cluster_list_t *cluster;
    cluster = list_head(cluster_list);
    while (cluster != NULL)
    {
        cluster_list_rm(cluster);
        cluster = list_head(cluster_list);
    }

    cluster_list_print();
    return;
}
/*---------------------------------------------------------------------------*/
void cluster_list_rm(cluster_list_t *cluster)
{
    if (cluster == NULL)
        return;
    PRINTF("removing cluster %d.%d \n",
           cluster->addr.u8[0], cluster->addr.u8[1]);

    list_remove(cluster_list, cluster);
    memb_free(&cluster_list_memb, cluster);
    num_clusters--;

    cluster_list_print();
    return;
}
/*---------------------------------------------------------------------------*/
static void cluster_list_print(void)
{
    PRINTF("cluster table:\n");
    cluster_list_t *r;
    for (r = cluster_list_head();
         r != NULL;
         r = cluster_list_next(r))
    {
        if (!r->CH)
        {
            PRINTF("node %d.%d CH addr %d.%d\n",
                   r->addr.u8[0], r->addr.u8[1],
                   r->ch_addr.u8[0], r->ch_addr.u8[1]);
        }
        else
        {
            PRINTF("CH node %d.%d\n",
                   r->addr.u8[0], r->addr.u8[1]);
        }
    }
    PRINTF("\n");
}