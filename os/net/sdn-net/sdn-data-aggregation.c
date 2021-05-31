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
#include "sdn-data-aggregation.h"

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

#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
/* The maximum number of pending packet per node */
#ifndef SDN_CONF_DATA_AGGREGTION_MAX
#define SDN_DATA_AGGREGTION_MAX 10
#else
#define SDN_DATA_AGGREGTION_MAX SDN_CONF_DATA_AGGREGTION_MAX
#endif /* SDN_CONF_MAX_PACKET_PER_NEIGHBOR */

#define MAX_QUEUED_PACKETS 30

/* The maximum number of sequence stored per node */
#ifdef SDN_CONF_MAX_SEQ_PER_NODE
#define SDN_MAX_SEQ_PER_NODE SDN_CONF_MAX_SEQ_PER_NODE
#else
#define SDN_MAX_SEQ_PER_NODE 3
#endif /* SDN_CONF_MAX_PACKET_PER_NEIGHBOR */

LIST(agg_list);
MEMB(agg_memb, sdn_data_aggregation_t, SDN_DATA_AGGREGTION_MAX);

MEMB(seq_list_memb, sdn_seq_list_t, MAX_QUEUED_PACKETS);

static int num_agg = 0;

#endif
/*---------------------------------------------------------------------------*/
void sdn_data_aggregation_init(void)
{
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
    memb_init(&agg_memb);
    list_init(agg_list);
#endif
}
/*---------------------------------------------------------------------------*/
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
sdn_data_aggregation_t *
sdn_data_aggregation_head(void)
{
    return list_head(agg_list);
}
#endif
/*---------------------------------------------------------------------------*/
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
sdn_data_aggregation_t *
sdn_data_aggregation_next(sdn_data_aggregation_t *r)
{
    return list_item_next(r);
}
#endif
/*---------------------------------------------------------------------------*/
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
int sdn_data_aggregation_num_packets(void)
{
    return num_agg;
}
#endif
/*---------------------------------------------------------------------------*/
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
sdn_data_aggregation_t *
sdn_data_aggregation_lookup(const linkaddr_t *addr)
{
    sdn_data_aggregation_t *found, *rt;

    if (addr == NULL)
    {
        return NULL;
    }

    found = NULL;
    // longestmatch = 0;
    for (rt = sdn_data_aggregation_head();
         rt != NULL;
         rt = sdn_data_aggregation_next(rt))
    {
        if (linkaddr_cmp(addr, &rt->addr))
            found = rt;
    }

    return found;
}
#endif
/*---------------------------------------------------------------------------*/
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
sdn_seq_list_t *
sdn_data_aggregation_lookup_list(const linkaddr_t *addr, const sdn_seq_list_t *data)
{
    sdn_data_aggregation_t *rt;
    sdn_seq_list_t *list_rt, *found;

    if (addr == NULL || data == NULL)
    {
        return NULL;
    }

    found = NULL;

    rt = sdn_data_aggregation_lookup(addr);

    if (rt != NULL)
    {
        for (list_rt = list_head(rt->seq_list);
             list_rt != NULL;
             list_rt = list_item_next(list_rt))
        {
            if (data->seq == list_rt->seq &&
                data->temp == list_rt->temp &&
                data->humidty == list_rt->humidty)
                found = list_rt;
        }
    }

    return found;
}
#endif
/*---------------------------------------------------------------------------*/
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
sdn_data_aggregation_t *
sdn_data_aggregation_add(const linkaddr_t *addr,
                         const sdn_seq_list_t *data)
{
    if (addr == NULL || data == NULL)
    {
        return NULL;
    }

    sdn_data_aggregation_t *rt;
    sdn_seq_list_t *list_rt, *dta;

    /* First make sure that we don't add scr node twice */
    rt = sdn_data_aggregation_lookup(addr);
    if (rt == NULL)
    {
        /* Allocate memory for new node */
        rt = memb_alloc(&agg_memb);
        if (rt == NULL)
        {
            PRINTF("Couldn't allocate more data packets\n");
            return NULL;
        }
        linkaddr_copy(&rt->addr, addr);
        list_add(agg_list, rt);
        LIST_STRUCT_INIT(rt, seq_list);
        // num_agg++;
    }

    /* We make sure we dont over pass the seq limit per node */
    if (list_length(rt->seq_list) < SDN_MAX_SEQ_PER_NODE)
    {
        /* We make sure data doesn't exist in the sequence list */
        list_rt = sdn_data_aggregation_lookup_list(addr, data);
        if (list_rt == NULL)
        {
            dta = memb_alloc(&seq_list_memb);

            if (dta == NULL)
                return NULL;

            dta->seq = data->seq;
            dta->temp = data->temp;
            dta->humidty = data->humidty;
            list_add(rt->seq_list, dta);

            num_agg++;

            PRINTF("aggregating (seq %u, temp %u hum %u node %d.%d)\n",
                   dta->seq,
                   dta->temp,
                   dta->humidty,
                   addr->u8[0], addr->u8[1]);
        }
    }
    else
    {
        PRINTF("list full.\n");
        return NULL;
    }

    return rt;
}
#endif
/*---------------------------------------------------------------------------*/
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
void sdn_data_aggregation_rm(sdn_data_aggregation_t *rt)
{
    if (rt == NULL)
        return;

    sdn_seq_list_t *dta = list_head(rt->seq_list);

    while (dta != NULL)
    {
        list_remove(rt->seq_list, dta);
        memb_free(&seq_list_memb, dta);
        num_agg--;
        dta = list_head(rt->seq_list);
    }

    /* Remove the scr node from the route list */
    if (list_length(rt->seq_list) == 0)
    {
        list_remove(agg_list, rt);
        memb_free(&agg_memb, rt);
    }

    return;
}
#endif
/*---------------------------------------------------------------------------*/
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
void sdn_data_aggregation_flush_all()
{
    sdn_data_aggregation_t *rt;
    rt = list_head(agg_list);
    while (rt != NULL)
    {
        sdn_data_aggregation_rm(rt);
        rt = list_head(agg_list);
    }
}
#endif