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
 *         Header for the Contiki/SD-WSN neighbor advertisement
 * \author
 *         Fernando Jurado <fdo.jurado@gmail.com>
 */

#include "net/sdn-net/sdn-advertisement.h"
#include "net/sdn-net/sdn-net.h"
#include "net/packetbuf.h"
#include "lib/random.h"
#include "net/sdn-net/sd-wsn.h"
#include "net/sdn-net/sdn.h"
#include "net/link-stats.h"
#include <string.h>
#include "net/sdn-net/sdn-ds-nbr.h"
#include "net/sdn-net/sdn-ds-route.h"
#include "net/sdn-net/sdn-neighbor-discovery.h"
#include "net/routing/routing.h"
#include "sdnbuf.h"
#if SDN_CONTROLLER
#include "sdn-controller/sdn-ds-node.h"
#include "sdn-controller/sdn-ds-config-routes.h"
#include "sdn-controller/sdn-ds-node-route.h"
#endif

#if SERIAL_SDN_CONTROLLER
#include "sdn-controller-serial/sdn-serial.h"
#include "sdn-controller-serial/sdn-serial-protocol.h"
#endif /* SERIAL_SDN_CONTROLLER */

#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
#include "sdn-energy.h"
#endif

/* Log configuration */
#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/** Period for uip-ds6 periodic task*/
#ifndef SDN_NA_CONF_PERIOD
#define SDN_NA_PERIOD (CLOCK_SECOND / 10)
#else
#define SDN_NA_PERIOD SDN_NA_CONF_PERIOD
#endif

#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
// static sdn_na_adv_t na_pkt; //Holds the rank value and the total rssi value to the controller
struct stimer na_timer_na; /**< NA timer, to schedule NA sending */
static uint16_t rand_time; /**< random time value for timers */
#endif

// int send_advertisement; // Send advertisement flag.

struct etimer na_timer_periodic;

/*---------------------------------------------------------------------------*/
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER) && SDN_WITH_TABLE_CHKSUM
static uint16_t
chksum(uint16_t sum, const uint8_t *data, uint16_t len)
{
    uint16_t t;
    const uint8_t *dataptr;
    const uint8_t *last_byte;

    dataptr = data;
    last_byte = data + len - 1;

    while (dataptr < last_byte)
    { /* At least two more bytes */
        t = (dataptr[0] << 8) + dataptr[1];
        sum += t;
        if (sum < t)
        {
            sum++; /* carry */
        }
        dataptr += 2;
    }

    if (dataptr == last_byte)
    {
        t = (dataptr[0] << 8) + 0;
        sum += t;
        if (sum < t)
        {
            sum++; /* carry */
        }
    }

    PRINTF("chksum, sum 0x%04x\n",
           sum);

    /* Return sum in host byte order. */
    return sum;
}
#endif
/*---------------------------------------------------------------------------*/
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER) && SDN_WITH_TABLE_CHKSUM
static uint16_t calculate_table_chksum(void)
{
    sdn_ds_route_t *r;
    const linkaddr_t *via;
    uint16_t sum = 0;
    // uint8_t data[4];

    typedef union
    {
        uint16_t u16[2];
        uint8_t u8[4];
    } data_t;

    data_t data;

    data.u16[0] = 0;
    data.u16[1] = 0;
    // struct sdn_ds_route_neighbor_routes *routes;
    for (r = sdn_ds_route_head();
         r != NULL;
         r = sdn_ds_route_next(r))
    {
        via = sdn_ds_route_nexthop(r);
        /* We only checksum routes that are not
         our neighbors */
        if (!linkaddr_cmp(&r->addr, via) || linkaddr_cmp(&r->addr, &ctrl_addr))
        {
            PRINTF("calculate_table_chksum: %d.%d - %d.%d\n",
                   r->addr.u8[0], r->addr.u8[1],
                   via->u8[0], via->u8[1]);
            data.u16[0] = r->addr.u16;
            data.u16[1] = via->u16;
            sum = chksum(sum, &data.u8[0], 4);
            PRINTF("sum 0x%04x\n",
                   sum);
        }
    }
    PRINTF("routing table chksum: sum 0x%04x\n",
           sum);
    return (sum == 0) ? 0xffff : sdnip_htons(sum);
}
#endif
/*---------------------------------------------------------------------------*/
#if SDN_DS_NBR_NOTIFICATIONS
static void
neighbor_callback(int event, const sdn_ds_nbr_t *nbr)
{
    if (event == SDN_DS_NBR_NOTIFICATION_ADD)
    {
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
        // PRINTF("adding neighbor, send NA\n");
        // send_advertisement = 1;
        // PRINTF("Sending NA because adding\n");
#endif
    }
    else if (event == SDN_DS_NBR_NOTIFICATION_RM)
    {
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
        // PRINTF("removing neighbor, send NA\n");
        // send_advertisement = 1;
        // PRINTF("Sending NA because removing\n");
#endif
    }
    else if (event == SDN_DS_NBR_NOTIFICATION_CH)
    {
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
        // PRINTF("neighbor changed, send NA\n");
        // send_advertisement = 1;
        // PRINTF("Sending NA because nb changed\n");
#endif
    }
    /* Give extra time to network settle down */
    // #if !SDN_CONTROLLER
    //     stimer_set(&na_timer_na, rand_time);
    // #endif
}
#endif
/*---------------------------------------------------------------------------*/
void sdn_na_init(void)
{
    etimer_set(&na_timer_periodic, SDN_NA_PERIOD);
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
    stimer_set(&na_timer_na, 2); /* wait to have a link local IP address */
#endif
/* callback function when neighbor removed */
#if SDN_DS_NBR_NOTIFICATIONS
    static struct sdn_ds_nbr_notification n;
    sdn_ds_nbr_notification_add(&n,
                                neighbor_callback);
#endif
    return;
}
/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER
void sdn_na_input(void)
{
    linkaddr_t from;
    /* Get the sender node address */
    from.u16 = sdnip_htons(SDN_IP_BUF->scr.u16);
    PRINTF("NA processing rcv from %d.%d\n",
           from.u8[0], from.u8[1]);
    uint8_t prev_ranks = 0, i;
    linkaddr_t neighbor_addr;
    uint8_t nxt_ranks = 0;
    int16_t sender_rank, sender_energy, neighbor_rssi, neighbor_rank;
    /* Calculate number of neighbors */
    uint8_t num_nb = SDN_CP_BUF->len / SDN_NA_LEN;
    /* Sender rank in host byte order */
    sender_rank = sdnip_htons(SDN_CP_BUF->rank);
    /* Sender energy in host byte order */
    sender_energy = sdnip_htons(SDN_CP_BUF->energy);
    /* # of previous and next ranks */
    PRINTF("num of neighbors %d sender rank %d sender energy %d\n", num_nb, sender_rank, sender_energy);
    for (i = 0; i < num_nb; i++)
    {
        neighbor_addr.u16 = sdnip_htons(SDN_NA_BUF(i)->addr.u16);
        neighbor_rssi = sdnip_htons(SDN_NA_BUF(i)->rssi);
        neighbor_rank = sdnip_htons(SDN_NA_BUF(i)->rank);
        /* count prev and nxt ranks */
        if (sender_rank > neighbor_rank)
            prev_ranks++;
        if (sender_rank < neighbor_rank)
            nxt_ranks++;
        PRINTF("nb = %d.%d rssi = %d rank= %d\n", neighbor_addr.u8[0], neighbor_addr.u8[1],
               neighbor_rssi,
               neighbor_rank);
#if SDN_CONTROLLER
        sdn_ds_node_route_add(&from, neighbor_rssi, &neighbor_addr);
#endif /* SDN_CONTROLLER */
    }
#if SDN_CONTROLLER
    sdn_ds_node_add(&from, sender_energy, sender_rank, prev_ranks, nxt_ranks, num_nb, 1);
#endif /* SDN_CONTROLLER */
/* Routing table checksum */
#if SDN_WITH_TABLE_CHKSUM
    sdn_ds_config_routes_chksum(&from, sdnip_htons(SDN_CP_BUF->rt_chksum));
#endif /* SDN_WITH_TABLE_CHKSUM */
}
#endif
/*---------------------------------------------------------------------------*/
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
static void send_na_output(void)
{
    const linkaddr_t *nxthop;
    const struct link_stats *stats;
    nxthop = NETSTACK_ROUTING.nexthop(&ctrl_addr);
    if (nxthop != NULL)
    {
        /* payload size */
        int8_t payload_size = SDN_NAPL_LEN * sdn_ds_nbr_num();
        PRINTF("Sending NA packet\n");
        /* IP packet */
        SDN_IP_BUF->vap = (0x01 << 5) | SDN_PROTO_NA;
        /* Total length */
        sdn_len = SDN_IPH_LEN + SDN_NAH_LEN + payload_size;
        SDN_IP_BUF->tlen = sdn_len;
        SDN_IP_BUF->ttl = 0x40;
        SDN_IP_BUF->scr.u16 = sdnip_htons(linkaddr_node_addr.u16);
        SDN_IP_BUF->dest.u16 = sdnip_htons(ctrl_addr.u16);
        SDN_IP_BUF->hdr_chksum = 0;
        SDN_IP_BUF->hdr_chksum = ~sdn_ipchksum();
        /* NA packet */
        SDN_NA_BUF->payload_len = payload_size;
        SDN_NA_BUF->rank = my_rank.rank;
        SDN_NA_BUF->energy = sdnip_htons((int16_t)energy);

        /* Put neighbor's info in payload */
        sdn_ds_nbr_t *nbr;
        uint8_t count = 0;

        for (nbr = sdn_ds_nbr_head(); nbr != NULL; nbr = sdn_ds_nbr_next(nbr))
        {
            SDN_NA_PAYLOAD(count)->nb_addr.u16 = sdnip_htons(nbr->addr.u16);
            // We get the ETX, RSSI values from link-stats.c
            stats = link_stats_from_lladdr(&nbr->addr);
            if (stats != NULL)
            {
                SDN_NA_PAYLOAD(count)->rssi = sdnip_htons(stats->rssi);
                SDN_NA_PAYLOAD(count)->etx = sdnip_htons(stats->etx);
            }
            else
            {
                SDN_NA_PAYLOAD(count)->rssi = sdnip_htons(0);
                SDN_NA_PAYLOAD(count)->etx = sdnip_htons(0);
            }
            count++;
        }

        SDN_NA_BUF->pkt_chksum = 0;
        SDN_NA_BUF->pkt_chksum = ~sdn_nachksum(payload_size);

        /* Update statistics */
        SDN_STAT(++sdn_stat.ip.sent);
        SDN_STAT(++sdn_stat.cp.adv);
        SDN_STAT(sdn_stat.cp.adv_bytes += sdn_len);

        print_buff(sdn_buf, sdn_len, true);

        sdnbuf_set_attr(SDNBUF_ATTR_MAX_MAC_TRANSMISSIONS, 3);

        sdn_ip_output(nxthop);
    }
}
#endif
/*---------------------------------------------------------------------------*/
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
static void sdn_send_na_periodic(void)
{
    // if (send_advertisement)
    // {
    send_na_output();
    // send_advertisement = 0;
    // }
    rand_time = SDN_MIN_NA_INTERVAL + random_rand() %
                                          (uint16_t)(SDN_MAX_NA_INTERVAL - SDN_MIN_NA_INTERVAL);
    PRINTF("Random time = %u\n", rand_time);
    stimer_set(&na_timer_na, rand_time);
}
#endif
/*---------------------------------------------------------------------------*/
void sdn_na_periodic(void)
{
/* Periodic ND sending */
// PRINTF("periodic.\n");
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
    if (stimer_expired(&na_timer_na) /* && (sdn_len == 0) */)
    {
        sdn_send_na_periodic();
    }
#endif
    etimer_reset(&na_timer_periodic);
}