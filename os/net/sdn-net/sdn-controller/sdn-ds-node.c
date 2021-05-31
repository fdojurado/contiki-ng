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
 *         Network nodes information table - equivalent ot nbr table
 * \author
 *         Fernando Jurado <fdo.jurado@gmail.com>
 */
#include "sdn-ds-node.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "sdn-ctrl-types.h"
#include "net/sdn-net/sdn.h"
#include "net/routing/routing.h"
#include "net/sdn-net/sd-wsn.h"
#include "sdn-ds-edge.h"
#include "net/sdn-net/sdn-network-config.h"

/* Log configuration */
#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#ifndef SDN_CONF_MAX_NA_INTERVAL
#define NODE_TIMEOUT 60 * 15
#else
#define NODE_TIMEOUT SDN_CONF_MAX_NA_INTERVAL * 3
#endif

#if SDN_DS_NODE_NOTIFICATIONS
LIST(notificationlist);
#endif

LIST(node_list);
MEMB(node_memb, sdn_ds_node_t, NODE_CACHES);

static int num_nodes = 0;

static sdn_ds_node_t *node;

static void
sdn_ds_node_print(void);

/*---------------------------------------------------------------------------*/
#if SDN_DS_NODE_NOTIFICATIONS
static void
call_node_callback(int event, const sdn_ds_node_t *node)
{
    struct sdn_ds_node_notification *n;
    for (n = list_head(notificationlist);
         n != NULL;
         n = list_item_next(n))
    {
        n->callback(event, node);
    }
}
/*---------------------------------------------------------------------------*/
void sdn_ds_node_notification_add(struct sdn_ds_node_notification *n,
                                  sdn_ds_node_notification_callback c)
{
    if (n != NULL && c != NULL)
    {
        n->callback = c;
        list_add(notificationlist, n);
    }
}
/*---------------------------------------------------------------------------*/
void sdn_ds_node_notification_rm(struct sdn_ds_node_notification *n)
{
    list_remove(notificationlist, n);
}
#endif
/*---------------------------------------------------------------------------*/
void sdn_ds_node_init(void)
{
    memb_init(&node_memb);
    list_init(node_list);
    // nbr_table_register(ds_node, (nbr_table_callback *)sdn_ds_node_rm);
}
/*---------------------------------------------------------------------------*/
sdn_ds_node_t *sdn_ds_node_add(const linkaddr_t *addr,
                               int16_t energy,
                               uint8_t rank,
                               uint8_t prev_ranks,
                               uint8_t next_ranks,
                               uint8_t total_nb,
                               uint8_t alive)
{
    /* Does this node route exists? */
    node = sdn_ds_node_lookup(addr);
    if (node == NULL)
    {
        /* New node route */
        node = memb_alloc(&node_memb);
        if (node == NULL)
        {
            PRINTF("Add drop node node from %d.%d\n",
                   addr->u8[0], addr->u8[1]);
            return NULL;
        }
        linkaddr_copy(&node->addr, addr);
        PRINTF("Adding new node %d.%d\n",
               addr->u8[0], addr->u8[1]);
        list_add(node_list, node);
        num_nodes++;
#if SDN_DS_NODE_NOTIFICATIONS
        call_node_callback(SDN_DS_NODE_NOTIFICATION_ADD, node);
#endif
    }
    node->energy = energy;
    node->rank = rank;
    node->prev_ranks = prev_ranks;
    node->next_ranks = next_ranks;
    node->total_ranks = next_ranks + prev_ranks;
    node->total_nb = total_nb;
    /* We only update alive flag
    if this has not been set */
    if (!alive)
    {
        if (!node->alive)
            node->alive = alive;
    }
    else
    {
        node->alive = alive;
    }

    /* set the lifetime of node */
    stimer_set(&node->lifetime, NODE_TIMEOUT);
    // ctimer_set(&node->timeout, NODE_TIMEOUT, sdn_ds_node_timer_rm, node);

    PRINTF("new entry to %d.%d with energy %d rank %d prev rank %d next rank %d total ranks %d and total nb %d alive %d\n ",
           node->addr.u8[0], node->addr.u8[1],
           node->energy,
           node->rank,
           node->prev_ranks,
           node->next_ranks,
           node->total_ranks,
           node->total_nb,
           node->alive);
    sdn_ds_node_print();
    return node;
}
/*---------------------------------------------------------------------------*/
// static void sdn_ds_node_timer_rm(void *n)
// {
//     sdn_ds_node_t *node = n;
//     sdn_ds_node_rm(node);
// }
/*---------------------------------------------------------------------------*/
void sdn_ds_node_rm(linkaddr_t *addr)
{
    /* Instead of removing we set the 
    remaining energy to 0. */
    PRINTF("removing node from ctrl table.\n");
    for (node = list_head(node_list);
         node != NULL;
         node = list_item_next(node))
    {
        if (linkaddr_cmp(addr, &node->addr))
        {
            node->energy = 0;
            node->rank = 0xFF;
            node->prev_ranks = 0;
            node->next_ranks = 0;
            node->total_ranks = 0;
            node->total_nb = 0;
            stimer_set(&node->lifetime, NODE_TIMEOUT);
#if SDN_DS_NODE_NOTIFICATIONS
            call_node_callback(SDN_DS_NODE_NOTIFICATION_RM, node);
#endif
            //             list_remove(node_list, node);
            //             memb_free(&node_memb, node);
            //             num_nodes--;
        }
    }
    // NETSTACK_ROUTING.neighbor_removed(nbr);
}
/*---------------------------------------------------------------------------*/
int sdn_ds_node_max_index(void)
{
    int n;
    linkaddr_t index;
    linkaddr_copy(&index, &ctrl_addr);

    for (node = list_head(node_list);
         node != NULL;
         node = list_item_next(node))
    {
        if (node->addr.u8[0] >= index.u8[0] &&
            node->addr.u8[1] >= index.u8[1])
            linkaddr_copy(&index, &node->addr);
    }
    if (index.u8[0] >= index.u8[1])
        n = index.u8[0];
    else
        n = index.u8[1];
    return n;
}
/*---------------------------------------------------------------------------*/
int sdn_ds_node_num(uint8_t *num_nodes)
{
    // return num_nodes;
    int num = 0;

    for (node = sdn_ds_node_head();
         node != NULL;
         node = sdn_ds_node_next(node))
    {
        if (node->energy > 0)
            num++;
    }

    /* Add the controller as well */
    *num_nodes = num + 1;
    return num;
}
/*---------------------------------------------------------------------------*/
unsigned long sdn_ds_node_energy_average()
{
    unsigned long energy = 0, num = 0;
    signed long avg;

    for (node = sdn_ds_node_head();
         node != NULL;
         node = sdn_ds_node_next(node))
    {
        if ((unsigned int)node->energy > 0)
        {
            energy = energy + node->energy;
            num++;
        }
    }

    avg = energy / num;

    PRINTF("Node energy average: %ld\n", avg);

    return avg;
}
/*---------------------------------------------------------------------------*/
unsigned long sdn_ds_node_total_energy()
{
    unsigned long energy = 0;

    for (node = sdn_ds_node_head();
         node != NULL;
         node = sdn_ds_node_next(node))
    {
        if ((unsigned int)node->energy > 0)
        {
            energy = energy + node->energy;
        }
    }

    PRINTF("Total WSN energy: %lu\n", energy);

    return energy;
}
/*---------------------------------------------------------------------------*/
sdn_ds_node_t *sdn_ds_node_head(void)
{
    return list_head(node_list);
}
/*---------------------------------------------------------------------------*/
int sdn_ds_node_max_rank(void)
{
    int max_rank = 0;

    for (node = list_head(node_list);
         node != NULL;
         node = list_item_next(node))
    {
        if (node->rank > max_rank)
            max_rank = node->rank;
    }
    return max_rank;
}
/*---------------------------------------------------------------------------*/
sdn_ds_node_t *sdn_ds_node_next(sdn_ds_node_t *node)
{
    return list_item_next(node);
}
/*---------------------------------------------------------------------------*/
sdn_ds_node_t *sdn_ds_node_lookup(const linkaddr_t *addr)
{
    PRINTF("Looking up for node node %d.%d\n",
           addr->u8[0], addr->u8[1]);

    if (addr == NULL)
    {
        return NULL;
    }
    for (node = list_head(node_list);
         node != NULL;
         node = list_item_next(node))
    {
        if (linkaddr_cmp(&node->addr, addr))
        {
            return node;
        }
    }
    PRINTF("Node not found\n");
    return NULL;
}
/*---------------------------------------------------------------------------*/
static void sdn_ds_node_print(void)
{
    PRINTF("node node table (%d nodes):\n", num_nodes);
    PRINTF(" node addr | energy (mJ) | rank | prev ranks | nxt ranks | total ranks | total nb | lifetime (s) | alive\n");
    for (node = sdn_ds_node_head();
         node != NULL;
         node = sdn_ds_node_next(node))
    {
        PRINTF(" \t\t %d.%d \t\t | \t\t %d \t\t | \t %d \t| \t\t\t\t %d \t\t\t\t|\t\t\t\t %d \t\t\t\t|\t\t\t\t\t %d \t\t\t\t\t|\t\t\t\t %d \t\t\t|\t\t\t %lu \t\t\t|\t\t\t %d\n",
               node->addr.u8[0], node->addr.u8[1],
               node->energy,
               node->rank,
               node->prev_ranks,
               node->next_ranks,
               node->total_ranks,
               node->total_nb,
               stimer_remaining(&node->lifetime),
               node->alive);
    }
    PRINTF("\n");
}
/*---------------------------------------------------------------------------*/
void sdn_ds_node_periodic(void)
{
    node = list_head(node_list);
    while (node != NULL)
    {
        /* Considering node is dead if its energy is less than 150L */
        if (node->alive &&
            node->energy < 150 &&
            !nc_computing())
        {
            PRINTF("refactoring killing node: %d.%d",
                   node->addr.u8[0], node->addr.u8[1]);
            node->alive = 0;
            sdn_ds_edge_remove_addr(&node->addr);
            stimer_set(&node->lifetime, 1);
            SDN_STAT(++sdn_stat.nodes.dead);
        }
        if (stimer_expired(&node->lifetime))
        {
            PRINTF("sdn_ds_node_periodic: node %d.%d",
                   node->addr.u8[0], node->addr.u8[1]);
            PRINTF(" lifetime expired\n");
            if (node->alive && !nc_computing())
            {
                PRINTF("node %d.%d has died\n", node->addr.u8[0], node->addr.u8[1]);
                node->alive = 0;
                sdn_ds_edge_remove_addr(&node->addr);
                SDN_STAT(++sdn_stat.nodes.dead);
            }
            else
            {
                sdn_ds_node_rm(&node->addr);
                node = list_head(node_list);
            }
        }
        else
        {
            node = list_item_next(node);
        }
    }
}