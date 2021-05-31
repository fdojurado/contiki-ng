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
#include "sdn-ds-config-routes.h"
#include "sdn-ds-node.h"
#include "net/sdn-net/sd-wsn.h"
#include "net/sdn-net/sdn-ds-nbr.h"

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

/* The maximum number of pending packet per node */
#ifdef SDN_CONF_MAX_PACKET_PER_NODE
#define SDN_MAX_PACKET_PER_NODE SDN_CONF_MAX_PACKET_PER_NODE
#else
#define SDN_MAX_PACKET_PER_NODE 10
#endif /* SDN_CONF_MAX_PACKET_PER_NEIGHBOR */

#define MAX_QUEUED_PACKETS 40

#if SDN_DS_CONFIG_ROUTES_NOTIFICATIONS
LIST(notificationlist);
#endif

LIST(configroutes);
MEMB(configmemb, sdn_ds_config_route_t, SDN_NODE_MAX_ROUTES);

MEMB(configlistmemb, sdn_ds_config_route_list_t, MAX_QUEUED_PACKETS);

static int num_routes = 0;

static void sdn_ds_config_routes_print(void);
/*---------------------------------------------------------------------------*/
#if SDN_DS_CONFIG_ROUTES_NOTIFICATIONS
static void
call_config_routes_callback(int event, const sdn_ds_config_route_t *route)
{
    struct sdn_ds_config_routes_notification *n;
    for (n = list_head(notificationlist);
         n != NULL;
         n = list_item_next(n))
    {
        n->callback(event, route);
    }
}
/*---------------------------------------------------------------------------*/
void sdn_ds_config_routes_notification_add(struct sdn_ds_config_routes_notification *n,
                                           sdn_ds_config_routes_notification_callback c)
{
    if (n != NULL && c != NULL)
    {
        n->callback = c;
        list_add(notificationlist, n);
    }
}
/*---------------------------------------------------------------------------*/
void sdn_ds_config_routes_notification_rm(struct sdn_ds_config_routes_notification *n)
{
    list_remove(notificationlist, n);
}
#endif
/*---------------------------------------------------------------------------*/
void sdn_ds_config_routes_init(void)
{
    memb_init(&configmemb);
    list_init(configroutes);
}
/*---------------------------------------------------------------------------*/
sdn_ds_config_route_t *
sdn_ds_config_routes_head(void)
{
    return list_head(configroutes);
}
/*---------------------------------------------------------------------------*/
sdn_ds_config_route_t *
sdn_ds_config_routes_next(sdn_ds_config_route_t *r)
{
    return list_item_next(r);
}
/*---------------------------------------------------------------------------*/
int sdn_ds_config_routes_num_routes(void)
{
    return num_routes;
}
/*---------------------------------------------------------------------------*/
int sdn_ds_config_routes_num_routes_node(const linkaddr_t *addr)

{
    uint8_t num = 0;
    sdn_ds_config_route_t *rt;
    sdn_ds_config_route_list_t *list_rt;
    for (rt = sdn_ds_config_routes_head();
         rt != NULL;
         rt = sdn_ds_config_routes_next(rt))
    {
        if (linkaddr_cmp(&rt->scr, addr))
        {
            for (list_rt = list_head(rt->rt_list);
                 list_rt != NULL;
                 list_rt = list_item_next(list_rt))
            {
                num++;
            }
        }
    }
    return num;
}
/*---------------------------------------------------------------------------*/
sdn_ds_config_route_t *
sdn_ds_config_routes_lookup(const linkaddr_t *scr)
{
    sdn_ds_config_route_t *found;
    sdn_ds_config_route_t *rt;

    PRINTF("Looking up scr for config route %d.%d\n",
           scr->u8[0], scr->u8[1]);

    if (scr == NULL)
    {
        return NULL;
    }

    found = NULL;
    // longestmatch = 0;
    for (rt = sdn_ds_config_routes_head();
         rt != NULL;
         rt = sdn_ds_config_routes_next(rt))
    {
        if (linkaddr_cmp(scr, &rt->scr))
            found = rt;
    }
    if (found != NULL)
    {
        PRINTF("scr found in config rt: %d.%d\n",
               scr->u8[0], scr->u8[1]);
    }
    else
    {
        PRINTF("No link found\n");
    }

    if (found != NULL && found != list_head(configroutes))
    {
        /* If we found a route, we put it at the start of the routeslist
       list. The list is ordered by how recently we looked them up:
       the least recently used route will be at the end of the
       list - for fast lookups (assuming multiple packets to the same node). */

        list_remove(configroutes, found);
        list_push(configroutes, found);
    }

    return found;
}
/*---------------------------------------------------------------------------*/
static sdn_ds_config_route_list_t *
sdn_ds_config_routes_lookup_list(const linkaddr_t *scr, const linkaddr_t *dest)
{
    sdn_ds_config_route_list_t *list_rt;
    sdn_ds_config_route_list_t *found;
    sdn_ds_config_route_t *rt;

    PRINTF("Looking up for config route: %d.%d -> %d.%d\n",
           scr->u8[0], scr->u8[1],
           dest->u8[0], dest->u8[1]);

    if (scr == NULL || dest == NULL)
    {
        return NULL;
    }

    found = NULL;

    for (rt = sdn_ds_config_routes_head();
         rt != NULL;
         rt = sdn_ds_config_routes_next(rt))
    {
        if (linkaddr_cmp(scr, &rt->scr))
        {
            for (list_rt = list_head(rt->rt_list);
                 list_rt != NULL;
                 list_rt = list_item_next(list_rt))
            {
                if (linkaddr_cmp(dest, &list_rt->dest))
                {
                    found = list_rt;
                }
            }
        }
    }

    if (found != NULL)
    {
        PRINTF("list found : %d.%d  - %d.%d (via %d.%d)\n",
               scr->u8[0], scr->u8[1],
               found->dest.u8[0], found->dest.u8[1],
               found->via.u8[0], found->via.u8[1]);
    }
    else
    {
        PRINTF("No link found\n");
    }

    return found;
}
/*---------------------------------------------------------------------------*/
sdn_ds_config_route_t *
sdn_ds_config_routes_add(const linkaddr_t *scr,
                         const linkaddr_t *dest,
                         const linkaddr_t *via)
{
    if (scr == NULL || dest == NULL || via == NULL || ((linkaddr_cmp(dest, via) && !linkaddr_cmp(dest, &linkaddr_node_addr))))
    {
        return NULL;
    }

    sdn_ds_config_route_list_t *list_rt, *dst;
    sdn_ds_config_route_t *rt;

    /* First make sure that we don't add scr node twice */
    rt = sdn_ds_config_routes_lookup(scr);
    if (rt == NULL)
    {
        /* Allocate memory for new node */
        rt = memb_alloc(&configmemb);
        if (rt == NULL)
        {
            PRINTF("Couldn't allocate more scr in config rt (%d.%d - %d.%d)\n",
                   scr->u8[0], scr->u8[1],
                   dest->u8[0], dest->u8[1]);
            return NULL;
        }
        linkaddr_copy(&rt->scr, scr);
        rt->ntwk_depth = 0;
        rt->acked = 0;
#if SDN_WITH_TABLE_CHKSUM
        rt->recv_chksum = 0;
#endif
        list_add(configroutes, rt);
        LIST_STRUCT_INIT(rt, rt_list);
    }
    /* We make sure we dont over pass the num rts limit per node */
    if (list_length(rt->rt_list) < SDN_MAX_PACKET_PER_NODE)
    {
        /* We make sure that we don't add dest node twice */
        list_rt = sdn_ds_config_routes_lookup_list(scr, dest);
        if (list_rt == NULL)
        {
            dst = memb_alloc(&configlistmemb);
            if (dst != NULL)
            {
                linkaddr_copy(&dst->via, via);
                linkaddr_copy(&dst->dest, dest);
#if SDN_WITH_TABLE_CHKSUM
                dst->current = 1;
#endif
                list_add(rt->rt_list, dst);
            }
            else
            {
                PRINTF("could not allocate route dest list.\n");
                return NULL;
            }
#if SDN_DS_CONFIG_ROUTES_NOTIFICATIONS
            call_config_routes_callback(SDN_DS_CONFIG_ROUTES_NOTIFICATION_ADD, rt);
#endif
            num_routes++;
        }
        else
        {
            // sdn_ds_config_routes_rm_dest(rt, dest, via);
            PRINTF("config rt already exists. fields updated: dest %d.%d via %d.%d\n",
                   list_rt->dest.u8[0], list_rt->dest.u8[1],
                   list_rt->via.u8[0], list_rt->via.u8[1]);
            linkaddr_copy(&list_rt->via, via);
            linkaddr_copy(&list_rt->dest, dest);
#if SDN_WITH_TABLE_CHKSUM
            list_rt->current = 1;
#endif
            // return NULL;
        }
    }

    PRINTF("adding config rt: %d.%d - %d.%d (via %d.%d)\n",
           scr->u8[0], scr->u8[1],
           dest->u8[0], dest->u8[1],
           via->u8[0], via->u8[1]);
    sdn_ds_config_routes_print();
    return rt;
}
/*---------------------------------------------------------------------------*/
#if SDN_WITH_TABLE_CHKSUM
void sdn_ds_config_routes_clear_current(void)
{
    sdn_ds_config_route_t *rt;
    sdn_ds_config_route_list_t *list_rt;
    for (rt = list_head(configroutes);
         rt != NULL;
         rt = list_item_next(rt))
    {
        for (list_rt = list_head(rt->rt_list);
             list_rt != NULL;
             list_rt = list_item_next(list_rt))
        {
            list_rt->current = 0;
        }
    }
}
#endif /* SDN_WITH_TABLE_CHKSUM */
/*---------------------------------------------------------------------------*/
#if SDN_WITH_TABLE_CHKSUM
void sdn_ds_config_routes_delete_current(const linkaddr_t *addr)
{
    if (addr == NULL)
        return;
    /* First make sure that we don't add scr node twice */
    PRINTF("Deleting routes not current for %d.%d.\n", addr->u8[0], addr->u8[1]);
    sdn_ds_config_route_t *rt;
    sdn_ds_config_route_list_t *dst;
    rt = sdn_ds_config_routes_lookup(addr);
    if (rt != NULL)
    {
        // sdn_ds_config_routes_rm(rt);
        // rt = list_head(configroutes);
        dst = list_head(rt->rt_list);

        while (dst != NULL)
        {
            if (!dst->current)
            {
                list_remove(rt->rt_list, dst);
                memb_free(&configlistmemb, dst);
                num_routes--;
                dst = list_head(rt->rt_list);
            }
            else
            {
                dst = list_item_next(dst);
            }
        }
        /* Remove the scr node from the route list */
        if (list_length(rt->rt_list) == 0)
        {
            list_remove(configroutes, rt);
            memb_free(&configmemb, rt);
        }
    }

    sdn_ds_config_routes_print();
}
#endif /* SDN_WITH_TABLE_CHKSUM */
/*---------------------------------------------------------------------------*/
#if SDN_WITH_TABLE_CHKSUM
sdn_ds_config_route_t *sdn_ds_config_routes_chksum(const linkaddr_t *scr, uint16_t chksum)
{
    if (scr == NULL)
        return NULL;
    sdn_ds_config_route_t *rt;

    /* First make sure that we don't add scr node twice */
    rt = sdn_ds_config_routes_lookup(scr);
    if (rt == NULL)
    {
        /* Allocate memory for new node */
        rt = memb_alloc(&configmemb);
        if (rt == NULL)
        {
            PRINTF("Couldn't allocate more scr in config rt %d.%d\n",
                   scr->u8[0], scr->u8[1]);
            return NULL;
        }
        linkaddr_copy(&rt->scr, scr);
        rt->ntwk_depth = 0;
        rt->acked = 0;
        list_add(configroutes, rt);
        LIST_STRUCT_INIT(rt, rt_list);
    }
    rt->recv_chksum = chksum;
    PRINTF("config rt %d.%d (chksum): 0x%04x\n",
           rt->scr.u8[0], rt->scr.u8[1],
           rt->recv_chksum);
    return rt;
}
#endif
/*---------------------------------------------------------------------------*/
void sdn_ds_config_routes_rm_dest(sdn_ds_config_route_t *route, const linkaddr_t *dest, const linkaddr_t *via)
{
    PRINTF("removing dest %d.%d (via %d.%d) from %d.%d \n",
           dest->u8[0], dest->u8[1],
           via->u8[0], via->u8[1],
           route->scr.u8[0], route->scr.u8[1]);
    sdn_ds_config_route_list_t *dst;
    if (route != NULL)
    {
        dst = list_head(route->rt_list);

        while (dst != NULL)
        {
            if (linkaddr_cmp(dest, &dst->dest) && linkaddr_cmp(via, &dst->via))
            {
                list_remove(route->rt_list, dst);
                memb_free(&configlistmemb, dst);
                num_routes--;
                PRINTF("free list rt: list length %d, free rts %d\n",
                       list_length(route->rt_list), memb_numfree(&configlistmemb));
                break;
            }
            else
            {
                dst = list_item_next(dst);
            }
        }
    }

    sdn_ds_config_routes_print();

    return;
}
/*---------------------------------------------------------------------------*/
void sdn_ds_config_routes_rm(sdn_ds_config_route_t *route)
{
    PRINTF("removing scr node (config rt)\n");
    sdn_ds_config_route_list_t *dst;
    if (route != NULL)
    {
        dst = list_head(route->rt_list);

        while (dst != NULL)
        {
            PRINTF("rmv config route: %d.%d - %d.%d (via %d.%d)\n",
                   route->scr.u8[0], route->scr.u8[1],
                   dst->dest.u8[0], dst->dest.u8[1],
                   dst->via.u8[0], dst->via.u8[1]);
            list_remove(route->rt_list, dst);
            memb_free(&configlistmemb, dst);
            num_routes--;
            PRINTF("free list rt: list length %d, free rts %d\n",
                   list_length(route->rt_list), memb_numfree(&configlistmemb));
            dst = list_head(route->rt_list);
        }
#if SDN_DS_CONFIG_ROUTES_NOTIFICATIONS
        call_config_routes_callback(SDN_DS_CONFIG_ROUTES_NOTIFICATION_RM, route);
#endif
        /* Remove the scr node from the route list */
        if (list_length(route->rt_list) == 0)
        {
            PRINTF("removing scr node %d.%d from config list\n",
                   route->scr.u8[0], route->scr.u8[1]);
            list_remove(configroutes, route);
            memb_free(&configmemb, route);
        }
    }

    sdn_ds_config_routes_print();

    return;
}
/*---------------------------------------------------------------------------*/
static void sdn_ds_config_routes_print(void)
{
    PRINTF("config routes (%d routes):\n", num_routes);
    sdn_ds_config_route_list_t *list_rt;
    sdn_ds_config_route_t *rt;
#if DEBUG
    sdn_ds_node_t *node;
#endif
    for (rt = list_head(configroutes);
         rt != NULL;
         rt = list_item_next(rt))
    {
#if DEBUG
        node = sdn_ds_node_lookup(&rt->scr);
#endif
#if SDN_WITH_TABLE_CHKSUM
        PRINTF("Printing rts of scr %d.%d (depth %d, acked %d, chksum 0x%04X, alive %d):\n",
               rt->scr.u8[0], rt->scr.u8[1],
               rt->ntwk_depth,
               rt->acked,
               rt->recv_chksum,
               node->alive);
#else
        PRINTF("Printing rts of scr %d.%d (depth %d, acked %d, alive %d):\n",
               rt->scr.u8[0], rt->scr.u8[1],
               rt->ntwk_depth,
               rt->acked,
               node->alive);
#endif
        for (list_rt = list_head(rt->rt_list);
             list_rt != NULL;
             list_rt = list_item_next(list_rt))
        {
#if SDN_WITH_TABLE_CHKSUM
            PRINTF(" %d.%d - %d.%d (via %d.%d, current %d)\n",
                   rt->scr.u8[0], rt->scr.u8[1],
                   list_rt->dest.u8[0], list_rt->dest.u8[1],
                   list_rt->via.u8[0], list_rt->via.u8[1],
                   list_rt->current);
#else
            PRINTF(" %d.%d - %d.%d (via %d.%d)\n",
                   rt->scr.u8[0], rt->scr.u8[1],
                   list_rt->dest.u8[0], list_rt->dest.u8[1],
                   list_rt->via.u8[0], list_rt->via.u8[1]);
#endif /* SDN_WITH_TABLE_CHKSUM */
        }
    }
    PRINTF("\n");
}
/*---------------------------------------------------------------------------*/
sdn_ds_config_route_t *sdn_ds_ntwk_depth_set(const linkaddr_t *addr, int8_t depth)
{
    sdn_ds_config_route_t *rt;

    rt = sdn_ds_config_routes_lookup(addr);

    if (rt != NULL)
        rt->ntwk_depth = depth;

    return rt;
}
/*---------------------------------------------------------------------------*/
sdn_ds_config_route_t *sdn_ds_config_routes_clear_flags(void)
{
    sdn_ds_config_route_t *rt;

    for (rt = list_head(configroutes);
         rt != NULL;
         rt = list_item_next(rt))
        rt->acked = 0;

    return rt;
}
/*---------------------------------------------------------------------------*/
void sdn_config_routes_flush_all()
{
    sdn_ds_config_route_t *rt;
    rt = list_head(configroutes);
    while (rt != NULL)
    {
        sdn_ds_config_routes_rm(rt);
        rt = list_head(configroutes);
    }
}
/*---------------------------------------------------------------------------*/
linkaddr_t *
sdn_config_routes_min_rank(void)
{
    sdn_ds_config_route_t *rt;
    linkaddr_t *addr;
    sdn_ds_node_t *node;
    uint8_t rank = 255;
    addr = NULL;
    for (rt = list_head(configroutes);
         rt != NULL;
         rt = list_item_next(rt))
    {
        node = sdn_ds_node_lookup(&rt->scr);
        if (node != NULL)
        {
            if (node->rank < rank)
            {
                rank = node->rank;
                addr = &rt->scr;
            }
        }
    }
    return addr;
}
/*---------------------------------------------------------------------------*/
linkaddr_t *
sdn_config_routes_min_depth(void)
{
    sdn_ds_config_route_t *rt;
    linkaddr_t *addr;
    int8_t depth = 127;

    addr = NULL;

    for (rt = list_head(configroutes);
         rt != NULL;
         rt = list_item_next(rt))
    {

        if (rt->ntwk_depth < depth && rt->acked != 1)
        {
            depth = rt->ntwk_depth;
            addr = &rt->scr;
        }
    }

    sdn_ds_config_routes_print();

    return addr;
}