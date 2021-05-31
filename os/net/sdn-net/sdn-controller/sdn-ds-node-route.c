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
#include "sdn-ds-node-route.h"
#include "sdn-ctrl-types.h"
#include "sdn-ds-node.h"
#include "net/sdn-net/sdn-ds-nbr.h"
#include "net/sdn-net/sdn.h"

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

#if SDN_DS_NODE_ROUTE_NOTIFICATIONS
LIST(notificationlist);
#endif

#define ROUTE_TIMEOUT (60 * 15)

LIST(node_route_list);
MEMB(node_route_memb, sdn_ds_route_node_t, SDN_NODE_MAX_ROUTES);

static int num_routes = 0;
static sdn_ds_route_node_t *rt;

/*---------------------------------------------------------------------------*/
#if SDN_DS_NODE_NOTIFICATIONS
static void
node_callback(int event, const sdn_ds_node_t *node)
{
    if (event == SDN_DS_NODE_NOTIFICATION_RM)
    {
        // PRINTF("node %d.%d",
        //        node->addr.u8[0], node->addr.u8[1]);
        // PRINTF(" died from network\n");
        // /* Delete all routes/links to this node */
        // // sdn_ds_node_route_rm_by_nexthop(&node->addr); //remove routes using this nb
        // sdn_ds_route_node_t *r;
        // r = list_head(node_route_list);
        // while (r != NULL)
        // {
        //     if (linkaddr_cmp(&node->addr, &r->scr) || linkaddr_cmp(&node->addr, &r->dest))
        //     {
        //         PRINTF("route_callback: removing link %d.%d - %d.%d (%ddBm) (%lus)\n",
        //                r->scr.u8[0], r->scr.u8[1],
        //                r->dest.u8[0], r->dest.u8[1],
        //                r->rssi,
        //                stimer_remaining(&r->lifetime));
        //         sdn_ds_node_route_rm(r);
        //         r = list_head(node_route_list);
        //     }
        //     else
        //     {
        //         r = list_item_next(r);
        //     }
        // }
    }
}
#endif
/*---------------------------------------------------------------------------*/
#if SDN_DS_NODE_ROUTE_NOTIFICATIONS
static void
call_route_callback(int event, const sdn_ds_route_node_t *link)
{
    struct sdn_ds_route_node_notification *n;
    for (n = list_head(notificationlist);
         n != NULL;
         n = list_item_next(n))
    {
        n->callback(event, link);
    }
}
/*---------------------------------------------------------------------------*/
void sdn_ds_node_route_notification_add(struct sdn_ds_route_node_notification *n,
                                        sdn_ds_node_route_notification_callback c)
{
    if (n != NULL && c != NULL)
    {
        n->callback = c;
        list_add(notificationlist, n);
    }
}
/*---------------------------------------------------------------------------*/
void sdn_ds_node_route_notification_rm(struct sdn_ds_route_node_notification *n)
{
    list_remove(notificationlist, n);
}
#endif
/*---------------------------------------------------------------------------*/
void sdn_ds_node_route_init(void)
{
    memb_init(&node_route_memb);
    list_init(node_route_list);
/* callback function when neighbor removed */
#if SDN_DS_NODE_NOTIFICATIONS
    static struct sdn_ds_node_notification n;
    sdn_ds_node_notification_add(&n,
                                 node_callback);
#endif
}
/*---------------------------------------------------------------------------*/
sdn_ds_route_node_t *
sdn_ds_node_route_head(void)
{
    return list_head(node_route_list);
}
/*---------------------------------------------------------------------------*/
sdn_ds_route_node_t *
sdn_ds_node_route_next(sdn_ds_route_node_t *r)
{
    return list_item_next(r);
}
/*---------------------------------------------------------------------------*/
int sdn_ds_node_route_num_routes(void)
{
    return num_routes;
}
/*---------------------------------------------------------------------------*/
sdn_ds_route_node_t *
sdn_ds_node_route_lookup(const linkaddr_t *scr, const linkaddr_t *dest)
{
    sdn_ds_route_node_t *found_route;
    // uint8_t longestmatch;

    PRINTF("Looking up for link: %d.%d - %d.%d\n",
           scr->u8[0], scr->u8[1],
           dest->u8[0], dest->u8[1]);

    if (scr == NULL || dest == NULL)
    {
        return NULL;
    }

    found_route = NULL;

    for (rt = sdn_ds_node_route_head();
         rt != NULL;
         rt = sdn_ds_node_route_next(rt))
    {
        if ((linkaddr_cmp(scr, &rt->scr) && linkaddr_cmp(dest, &rt->dest)) ||
            (linkaddr_cmp(dest, &rt->scr) && linkaddr_cmp(scr, &rt->dest)))
        {
            found_route = rt;
        }
    }

    if (found_route != NULL)
    {
        PRINTF("Link found: %d.%d - %d.%d\n",
               scr->u8[0], scr->u8[1],
               dest->u8[0], dest->u8[1]);
    }
    else
    {
        PRINTF("No link found\n");
    }

    if (found_route != NULL && found_route != list_head(node_route_list))
    {
        /* If we found a route, we put it at the start of the routeslist
       list. The list is ordered by how recently we looked them up:
       the least recently used route will be at the end of the
       list - for fast lookups (assuming multiple packets to the same node). */

        list_remove(node_route_list, found_route);
        list_push(node_route_list, found_route);
    }

    return found_route;
}
/*---------------------------------------------------------------------------*/
sdn_ds_route_node_t *
sdn_ds_node_route_add(const linkaddr_t *scr, int16_t rssi,
                      const linkaddr_t *dest)
{
    if (scr == NULL || dest == NULL)
    {
        return NULL;
    }

    /* First make sure that we don't add a route twice. If we find an
     existing route for our destination, we'll delete the old
     one first. */
    rt = sdn_ds_node_route_lookup(scr, dest);
    if (rt == NULL)
    {
        /* New node route */
        rt = memb_alloc(&node_route_memb);
        if (rt == NULL)
        {
            PRINTF("Couldn't allocate more links (%d.%d-%d.%d)\n",
                   scr->u8[0], scr->u8[1],
                   dest->u8[0], dest->u8[1]);
            return NULL;
        }
        linkaddr_copy(&rt->scr, scr);
        linkaddr_copy(&rt->dest, dest);
        PRINTF("Adding new link %d.%d-%d.%d\n",
               scr->u8[0], scr->u8[1],
               dest->u8[0], dest->u8[1]);
        list_add(node_route_list, rt);
        num_routes++;
#if SDN_DS_NODE_ROUTE_NOTIFICATIONS
        call_route_callback(SDN_DS_NODE_ROUTE_NOTIFICATION_ADD, rt);
#endif
        /* Make sure these two nodes exist in ctrl's node info, 
        if not create a empty node */
        sdn_ds_node_t *node;
        node = sdn_ds_node_lookup(scr);
        if (node == NULL && !linkaddr_cmp(&ctrl_addr, scr))
            sdn_ds_node_add(scr, 0, 0xFF, 0, 0, 0, 0); //Set rank to maximum
        node = sdn_ds_node_lookup(dest);
        if (node == NULL && !linkaddr_cmp(&ctrl_addr, dest))
            sdn_ds_node_add(dest, 0, 0xFF, 0, 0, 0, 0); //Set rank to maximum
    }
    /* update rssi */
    rt->rssi = rssi;
    /* set the lifetime of node */
    stimer_set(&rt->lifetime, ROUTE_TIMEOUT);

    PRINTF("updating link: %d.%d - %d.%d (%ddBm) (%lus)\n",
           scr->u8[0], scr->u8[1],
           dest->u8[0], dest->u8[1],
           rt->rssi,
           stimer_remaining(&rt->lifetime));
    sdn_ds_route_node_print();
    return rt;
}
/*---------------------------------------------------------------------------*/
void sdn_ds_node_route_rm(sdn_ds_route_node_t *route)
{
    PRINTF("removing link.\n");
    for (rt = list_head(node_route_list);
         rt != NULL;
         rt = list_item_next(rt))
    {
        if ((linkaddr_cmp(&route->scr, &rt->scr) && linkaddr_cmp(&route->dest, &rt->dest)) ||
            (linkaddr_cmp(&route->dest, &rt->scr) && linkaddr_cmp(&route->scr, &rt->dest)))
        {
#if SDN_DS_NODE_ROUTE_NOTIFICATIONS
            call_route_callback(SDN_DS_NODE_ROUTE_NOTIFICATION_RM, rt);
#endif
            list_remove(node_route_list, rt);
            memb_free(&node_route_memb, rt);
            num_routes--;
        }
    }
    return;
}
/*---------------------------------------------------------------------------*/
void sdn_ds_route_node_print(void)
{
    PRINTF("links table (%d links):\n", num_routes);
    for (rt = list_head(node_route_list);
         rt != NULL;
         rt = list_item_next(rt))
    {
        PRINTF("%d.%d - %d.%d (%ddBm) (%lus)\n",
               rt->scr.u8[0], rt->scr.u8[1],
               rt->dest.u8[0], rt->dest.u8[1],
               rt->rssi,
               stimer_remaining(&rt->lifetime));
    }
    PRINTF("\n");
}
/*---------------------------------------------------------------------------*/
void sdn_ds_route_node_print_neighbors(const linkaddr_t *from)
{
    PRINTF("link table: %d.%d\n",
           from->u8[0], from->u8[1]);
    for (rt = list_head(node_route_list);
         rt != NULL;
         rt = list_item_next(rt))
    {
        if (linkaddr_cmp(from, &rt->scr) ||
            linkaddr_cmp(from, &rt->dest))
        {

            PRINTF("%d.%d - %d.%d (%ddBm) (%lus)\n",
                   rt->scr.u8[0], rt->scr.u8[1],
                   rt->dest.u8[0], rt->dest.u8[1],
                   rt->rssi,
                   stimer_remaining(&rt->lifetime));
        }
    }
}
/*---------------------------------------------------------------------------*/
void sdn_ds_node_route_periodic(void)
{
    rt = list_head(node_route_list);
    while (rt != NULL)
    {
        if (stimer_expired(&rt->lifetime))
        {
            PRINTF("lifetime expired: %d.%d - %d.%d (%ddBm) (%lus)\n",
                   rt->scr.u8[0], rt->scr.u8[1],
                   rt->dest.u8[0], rt->dest.u8[1],
                   rt->rssi,
                   stimer_remaining(&rt->lifetime));
            sdn_ds_node_route_rm(rt);
            rt = list_head(node_route_list);
        }
        else
        {
            rt = list_item_next(rt);
        }
    }
}