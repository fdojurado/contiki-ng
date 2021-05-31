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
#include "sdn-ds-edge.h"
#include "net/sdn-net/sdn-ds-nbr.h"

#include "lib/list.h"
#include "lib/memb.h"
#include "net/nbr-table.h"

/* Log configuration */
#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#ifdef SDN_CONF_EDGE_MAX
#define SDN_EDGE_MAX SDN_CONF_EDGE_MAX
#else
#define SDN_EDGE_MAX 20
#endif

/* Each edge is repressented by a uip_ds6_route_t structure and
   memory for each edge is allocated from the edgememb memory
   block. These routes are maintained on the edgelist. */
LIST(edgelist);
MEMB(edgememb, sdn_ds_edge_t, SDN_EDGE_MAX);

static int num_edges = 0;
// static void rm_routelist_callback(nbr_table_item_t *ptr);

static void sdn_ds_edge_print(void);

/*---------------------------------------------------------------------------*/
void sdn_ds_edge_init(void)
{
    memb_init(&edgememb);
    list_init(edgelist);
}
/*---------------------------------------------------------------------------*/
sdn_ds_edge_t *
sdn_ds_edge_head(void)
{
    return list_head(edgelist);
}
/*---------------------------------------------------------------------------*/
sdn_ds_edge_t *
sdn_ds_edge_tail(void)
{
    return list_tail(edgelist);
}
/*---------------------------------------------------------------------------*/
sdn_ds_edge_t *
sdn_ds_edge_next(sdn_ds_edge_t *r)
{
    return list_item_next(r);
}
/*---------------------------------------------------------------------------*/
int sdn_ds_edge_num_edges(void)
{
    return num_edges;
}
/*---------------------------------------------------------------------------*/
sdn_ds_edge_t *
sdn_ds_edge_lookup(const linkaddr_t *scr, const linkaddr_t *dest)
{
    sdn_ds_edge_t *r;
    sdn_ds_edge_t *found_edge;
    // uint8_t longestmatch;

    PRINTF("Looking up for edge: %d.%d - %d.%d\n",
           scr->u8[0], scr->u8[1],
           dest->u8[0], dest->u8[1]);

    if (scr == NULL || dest == NULL)
    {
        return NULL;
    }

    found_edge = NULL;
    // longestmatch = 0;
    for (r = sdn_ds_edge_head();
         r != NULL;
         r = sdn_ds_edge_next(r))
    {
        if (linkaddr_cmp(&r->scr, scr) && linkaddr_cmp(&r->dest, dest))
        {
            found_edge = r;
        }
        if (linkaddr_cmp(&r->dest, scr) && linkaddr_cmp(&r->scr, dest))
        {
            found_edge = r;
        }
    }

    if (found_edge != NULL)
    {
        PRINTF("Edge found: %d.%d - %d.%d\n",
               scr->u8[0], scr->u8[1],
               dest->u8[0], dest->u8[1]);
    }
    else
    {
        PRINTF("No edge found\n");
    }

    if (found_edge != NULL && found_edge != list_head(edgelist))
    {
        /* If we found a edge, we put it at the start of the routeslist
       list. The list is ordered by how recently we looked them up:
       the least recently used edge will be at the end of the
       list - for fast lookups (assuming multiple packets to the same node). */

        list_remove(edgelist, found_edge);
        list_push(edgelist, found_edge);
    }

    return found_edge;
}
/*---------------------------------------------------------------------------*/
sdn_ds_edge_t *
sdn_ds_edge_add(const linkaddr_t *scr, const linkaddr_t *dest)
{
    if (scr == NULL || dest == NULL)
    {
        return NULL;
    }
    sdn_ds_edge_t *edge;

    edge = sdn_ds_edge_lookup(scr, dest);

    /* First make sure that we don't add edge twice */
    if (edge == NULL)
    {
        /* Allocate memory for new node */
        edge = memb_alloc(&edgememb);
        if (edge == NULL)
        {
            PRINTF("Couldn't allocate more edges (%d.%d - %d.%d)\n",
                   scr->u8[0], scr->u8[1],
                   dest->u8[0], dest->u8[1]);
            return NULL;
        }
        linkaddr_copy(&edge->scr, scr);
        linkaddr_copy(&edge->dest, dest);
        list_push(edgelist, edge);
        num_edges++;
    }

    sdn_ds_edge_print();
    return edge;
}
/*---------------------------------------------------------------------------*/
void sdn_ds_edge_flush()
{
    sdn_ds_edge_t *edge;
    edge = list_head(edgelist);
    while (edge != NULL)
    {
        sdn_ds_edge_rm(edge);
        edge = list_head(edgelist);
    }

    sdn_ds_edge_print();
    return;
}
/*---------------------------------------------------------------------------*/
void sdn_ds_edge_remove_addr(const linkaddr_t *addr)
{
    sdn_ds_edge_t *edge;

    edge = list_head(edgelist);
    while (edge != NULL)
    {
        if (linkaddr_cmp(&edge->scr, addr) || linkaddr_cmp(&edge->dest, addr))
        {
            sdn_ds_edge_rm(edge);
            edge = list_head(edgelist);
        }
        else
        {
            edge = list_item_next(edge);
        }
    }

    sdn_ds_edge_print();
    return;
}
/*---------------------------------------------------------------------------*/
void sdn_ds_edge_rm(sdn_ds_edge_t *edge)
{
    if (edge == NULL)
        return;
    PRINTF("removing edge %d.%d - %d.%d\n",
           edge->scr.u8[0], edge->scr.u8[1],
           edge->dest.u8[0], edge->dest.u8[1]);

    list_remove(edgelist, edge);
    memb_free(&edgememb, edge);
    num_edges--;

    sdn_ds_edge_print();
    return;
}
/*---------------------------------------------------------------------------*/
static void sdn_ds_edge_print(void)
{
    PRINTF("edge table (%d edges):\n", num_edges);
    sdn_ds_edge_t *r;
    for (r = sdn_ds_edge_head();
         r != NULL;
         r = sdn_ds_edge_next(r))
    {
        PRINTF("%d.%d - %d.%d\n",
               r->scr.u8[0], r->scr.u8[1],
               r->dest.u8[0], r->dest.u8[1]);
    }
    PRINTF("\n");
}