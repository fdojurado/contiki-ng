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
 *         Header for the Contiki/SD-WSN interface
 * \author
 *         Fernando Jurado <fdo.jurado@gmail.com>
 */

#include "sdn-data-packets.h"
#include "sdn-data-aggregation.h"
#include "sdn-ds-route.h"
#include "sdn-net.h"
#include "net/routing/routing.h"
#include "net/packetbuf.h"
#include "lib/random.h"
#include "sd-wsn.h"
#include "sdn.h"
#include <string.h>
#include "sdnbuf.h"

#include "lib/list.h"
#include "lib/memb.h"

/* Log configuration */
#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/** Period for uip-ds6 periodic task*/
#ifndef SDN_DATA_CONF_PERIOD
#define SDN_DATA_PERIOD (CLOCK_SECOND / 10)
#else
#define SDN_DATA_PERIOD SDN_DATA_CONF_PERIOD
#endif

#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
struct etimer data_timer_periodic;
struct stimer data_timer_send; /**< ND timer, to schedule ND sending */
static uint16_t rand_time;     /**< random time value for timers */
static uint8_t seq = 0;
#endif

#if SDN_CONTROLLER || SERIAL_SDN_CONTROLLER

#ifdef SDN_DS_CONF_MAX_NODE_CACHES
#define NODE_CACHES SDN_DS_CONF_MAX_NODE_CACHES + 1
#else
#define NODE_CACHES 10
#endif

typedef struct pdr
{
    struct pdr *next;
    linkaddr_t addr;
    uint16_t seq;
    uint16_t last_seq;
    uint16_t num_seqs;
    // unsigned long pdr;
} pdr_t;

LIST(pdr_list);
MEMB(pdr_memb, pdr_t, NODE_CACHES);

#endif /* SDN_CONTROLLER */

/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER || SERIAL_SDN_CONTROLLER
pdr_t *
sdn_data_pdr_head(void)
{
    return list_head(pdr_list);
}
#endif
/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER || SERIAL_SDN_CONTROLLER
pdr_t *
sdn_data_pdr_next(pdr_t *r)
{
    return list_item_next(r);
}
#endif
/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER || SERIAL_SDN_CONTROLLER
pdr_t *
sdn_data_pdr_lookup(const linkaddr_t *addr)
{
    pdr_t *found;
    pdr_t *rt;

    if (addr == NULL)
    {
        return NULL;
    }

    found = NULL;

    for (rt = sdn_data_pdr_head();
         rt != NULL;
         rt = sdn_data_pdr_next(rt))
    {
        if (linkaddr_cmp(&rt->addr, addr))
        {
            found = rt;
        }
    }

    return found;
}
#endif
/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER || SERIAL_SDN_CONTROLLER
pdr_t *
sdn_data_pdr_add(const linkaddr_t *addr, uint16_t seq)
{
    if (addr == NULL)
    {
        return NULL;
    }

    pdr_t *rt;

    /* First make sure that we don't add a route twice. If we find an
     existing route for our destination, we'll delete the old
     one first. */
    rt = sdn_data_pdr_lookup(addr);
    if (rt == NULL)
    {
        /* New node route */
        rt = memb_alloc(&pdr_memb);
        if (rt == NULL)
        {
            PRINTF("Couldn't allocate more pdr\n");
            return NULL;
        }
        linkaddr_copy(&rt->addr, addr);
        rt->seq = 0;
        rt->last_seq = 0;
        rt->num_seqs = 0;
        list_add(pdr_list, rt);
    }
    /* avoid repeated seq */
    if (seq == rt->last_seq)
        return rt;
    /* last sequence */
    rt->last_seq = seq;
    /* num of sequences */
    rt->num_seqs += 1;
    /* pdr */
    // rt->pdr = rt->num_seqs * 100L / rt->last_seq;

    PRINTF("1, %d, %u, %u, , , , , , , , ,\n",
           rt->addr.u8[0],
           rt->last_seq,
           rt->num_seqs);

    // PRINTF("last seq %d num seqs %d PDR %lu%% (%d.%d)\n",
    //        rt->last_seq,
    //        rt->num_seqs,
    //        rt->pdr,
    //        rt->addr.u8[0], rt->addr.u8[1]);

    return rt;
}
#endif
/*---------------------------------------------------------------------------*/
void sdn_data_init(void)
{
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
    etimer_set(&data_timer_periodic, SDN_DATA_PERIOD);
    stimer_set(&data_timer_send, 2); /* wait to have a link local IP address */
#endif

#if SDN_CONTROLLER || SERIAL_SDN_CONTROLLER
    memb_init(&pdr_memb);
    list_init(pdr_list);
#endif
    return;
}
/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER || SERIAL_SDN_CONTROLLER
void sdn_data_input(void)
{
    linkaddr_t sender;
    uint16_t seq;
    uint8_t num;

    num = SDN_DATA_HDR_BUF->len / SDN_DATA_LEN;

    if (num == 1)
    {
        sender.u16 = sdnip_htons(SDN_IP_BUF->scr.u16);
        seq = sdnip_htons(SDN_DATA_BUF(0)->seq);
        // temp = SDN_DATA_BUF->temp;
        // hum = SDN_DATA_BUF->humidty;

        sdn_data_pdr_add(&sender, seq);
        return;
    }

    // PRINTF("aggregated data packet received.\n");

    uint8_t i;

    for (i = 0; i < num; i++)
    {
        sender.u16 = sdnip_htons(SDN_DATA_BUF(i)->addr.u16);
        seq = sdnip_htons(SDN_DATA_BUF(i)->seq);
        sdn_data_pdr_add(&sender, seq);
    }

    return;
}
#endif
/*---------------------------------------------------------------------------*/
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
static void send_data_output(void)
{
    const linkaddr_t *nxthop;
    uint8_t aggregate = 0;
    nxthop = NETSTACK_ROUTING.nexthop(&ctrl_addr);
    if (nxthop != NULL)
    {
        SDN_IP_BUF->ttl = 0x40;

        SDN_IP_BUF->proto = SDN_PROTO_DATA;

        SDN_IP_BUF->scr.u16 = sdnip_htons(linkaddr_node_addr.u16);

        SDN_IP_BUF->dest.u16 = sdnip_htons(ctrl_addr.u16);

        /* Data packet */
        seq++;
        if (!cluster_head || (sdn_data_aggregation_num_packets() == 0))
        {
            SDN_IP_BUF->vahl = (0x03 << 4) | SDN_IPH_LEN;

            SDN_DATA_HDR_BUF->len = SDN_DATA_LEN;
            SDN_DATA_BUF(0)->addr.u16 = sdnip_htons(linkaddr_node_addr.u16);
            SDN_DATA_BUF(0)->seq = sdnip_htons(seq);
            SDN_DATA_BUF(0)->temp = sdnip_htons(random_rand() % (uint8_t)(0xFFFF));
            SDN_DATA_BUF(0)->humidty = sdnip_htons(random_rand() % (uint8_t)(0xFFFF));

            PRINTF("1, %d, %u, %u, , , , , , , , ,\n",
                   linkaddr_node_addr.u8[0],
                   seq,
                   SDN_DATA_BUF(0)->temp);
        }
        else
        {
            SDN_IP_BUF->vahl = (0x01 << 5) | SDN_IPH_LEN;
            /* aggregate data available at the time */
            SDN_DATA_HDR_BUF->len = SDN_DATA_LEN * (sdn_data_aggregation_num_packets() + 1);
            SDN_DATA_BUF(0)->addr.u16 = sdnip_htons(linkaddr_node_addr.u16);
            SDN_DATA_BUF(0)->seq = sdnip_htons(seq);
            SDN_DATA_BUF(0)->temp = sdnip_htons(random_rand() % (uint8_t)(0xFFFF));
            SDN_DATA_BUF(0)->humidty = sdnip_htons(random_rand() % (uint8_t)(0xFFFF));

            PRINTF("1, %d, %u, %u, , , , , , , , ,\n",
                   linkaddr_node_addr.u8[0],
                   seq,
                   SDN_DATA_BUF(0)->temp);

            uint8_t i = 1;
            sdn_data_aggregation_t *rt;
            sdn_seq_list_t *dta;

            for (rt = sdn_data_aggregation_head();
                 rt != NULL;
                 rt = sdn_data_aggregation_next(rt))
            {

                for (dta = list_head(rt->seq_list);
                     dta != NULL;
                     dta = list_item_next(dta))
                {
                    SDN_DATA_BUF(i)->addr.u16 = sdnip_htons(rt->addr.u16);
                    SDN_DATA_BUF(i)->seq = sdnip_htons(dta->seq);
                    SDN_DATA_BUF(i)->temp = sdnip_htons(dta->temp);
                    SDN_DATA_BUF(i)->humidty = sdnip_htons(dta->humidty);
                    PRINTF("1, %d, %u, %u, , , , , , , , ,\n",
                           rt->addr.u8[0],
                           dta->seq,
                           dta->temp);
                    i++;
                }
            }
            aggregate = 1;
        }

        /* Total length */
        sdn_len = SDN_IPH_LEN + SDN_DATAH_LEN + SDN_DATA_HDR_BUF->len;
        SDN_IP_BUF->len = sdn_len;

        SDN_IP_BUF->ipchksum = 0;
        SDN_IP_BUF->ipchksum = ~sdn_ipchksum();

        /* Update statistics */
        SDN_STAT(++sdn_stat.ip.sent);
        if (!aggregate)
        {
            SDN_STAT(++sdn_stat.data.sent_nagg);
            SDN_STAT(sdn_stat.data.sent_nagg_bytes += sdn_len);
        }
        else
        {
            SDN_STAT(++sdn_stat.data.sent_agg);
            SDN_STAT(sdn_stat.data.sent_agg_bytes += sdn_len);
        }

        print_buff(sdn_buf, sdn_len, true);

        sdnbuf_set_attr(SDNBUF_ATTR_MAX_MAC_TRANSMISSIONS, 0);

        sdn_ip_output(nxthop);

        sdn_data_aggregation_flush_all();
    }
}
#endif
/*---------------------------------------------------------------------------*/
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
static void sdn_send_nd_periodic(void)
{
    send_data_output();
    rand_time = SDN_MIN_DATA_PACKET_INTERVAL + random_rand() %
                                                   (uint16_t)(SDN_DATA_PACKET_INTERVAL - SDN_MIN_DATA_PACKET_INTERVAL);
    stimer_set(&data_timer_send, rand_time);
}
#endif
/*---------------------------------------------------------------------------*/
void sdn_data_periodic(void)
{
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
    if (stimer_expired(&data_timer_send) /* && (sdn_len == 0) */)
    {
        sdn_send_nd_periodic();
    }
    etimer_reset(&data_timer_periodic);
#endif
}