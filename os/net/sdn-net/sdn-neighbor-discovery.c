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

#include "sdn-neighbor-discovery.h"
#include "sdn-net.h"
#include "net/packetbuf.h"
#include "lib/random.h"
#include "sd-wsn.h"
#include "sdn.h"
#include <string.h>
#include "sdn-ds-nbr.h"
#include "sdn-ds-route.h"
#include "sdnbuf.h"

/* Log configuration */
#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/** Period for uip-ds6 periodic task*/
#ifndef SDN_ND_CONF_PERIOD
#define SDN_ND_PERIOD (CLOCK_SECOND / 10)
#else
#define SDN_ND_PERIOD SDN_ND_CONF_PERIOD
#endif

#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
sdn_rank_t my_rank; // Holds the rank value and the total rssi value to the controller
#endif

struct etimer nd_timer_periodic;
struct stimer nd_timer_na; /**< ND timer, to schedule ND sending */
static uint16_t rand_time; /**< random time value for timers */

/*---------------------------------------------------------------------------*/
#if SDN_DS_NBR_NOTIFICATIONS
static void
neighbor_callback(int event, const sdn_ds_nbr_t *nbr)
{
    if (event == SDN_DS_NBR_NOTIFICATION_RM)
    {
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
        PRINTF("removing neighbor %d.%d, check whether is gtw to ctrl\n",
               nbr->addr.u8[0], nbr->addr.u8[1]);
        if (linkaddr_cmp(&my_rank.addr, &nbr->addr))
        {
            PRINTF("removing gtw to ctrl\n");
            my_rank.rank = 0xff;
            my_rank.rssi = 0;
            /* Remove route to ctrl */
            sdn_ds_route_rm_by_nexthop(&ctrl_addr);
        }
#endif
    }
}
#endif
/*---------------------------------------------------------------------------*/
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
static void update_rank(int16_t rssi, uint8_t rank, const linkaddr_t *from)
{
    // if (my_rank.rank > 1)
    if (sdn_ds_route_add(&ctrl_addr, rssi, from, SDN_NODE) == NULL)
        PRINTF("rank route could not be updated, locked by ctrl\n");
    // else
    // {
    my_rank.rank = rank + 1;
    my_rank.rssi = rssi;
    linkaddr_copy(&my_rank.addr, from);
    PRINTF("rank updated: rank %d total rssi %d\n", my_rank.rank, my_rank.rssi);
    PRINTF(" gw address = %d.%d\n", my_rank.addr.u8[1], my_rank.addr.u8[0]);
    // }
}
#endif
/*---------------------------------------------------------------------------*/
void sdn_nd_init(void)
{
    etimer_set(&nd_timer_periodic, SDN_ND_PERIOD);
    stimer_set(&nd_timer_na, 2); /* wait to have a link local IP address */
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
    my_rank.rank = 0xff; /* Sensor node */
    my_rank.rssi = 0x00;
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
void sdn_nd_input(void)
{
    int16_t ndRank, ndRssi, rssi;
    rssi = (int16_t)sdn_net_get_last_rssi();
    ndRank = sdn_ntohs(SDN_ND_BUF->rank);
    ndRssi = sdn_ntohs(SDN_ND_BUF->rssi);

    PRINTF("Processing ND packet with rcv rssi %d and rank %d and rssi to ctrl %d\n",
           rssi,
           ndRank,
           ndRssi);

#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
    /* Check whether the ND message is
     * from the gateway. If it is, we need
     * to update the rank
     * */
    if (linkaddr_cmp((linkaddr_t *)packetbuf_addr(PACKETBUF_ADDR_SENDER),
                     &my_rank.addr))
    {
        if (ndRank == 255)
        {
            my_rank.rank = 0xff;
            my_rank.rssi = 0;
            /* Remove route to ctrl */
            sdn_ds_route_rm_by_nexthop(&ctrl_addr);
        }
        else
        {
            update_rank(ndRssi + rssi,
                        ndRank,
                        (linkaddr_t *)packetbuf_addr(PACKETBUF_ADDR_SENDER));
        }
    }
    else if (ndRank == my_rank.rank - 1)
    {
        PRINTF("rank equal.\n");
        /* acc rssi is greater? */
        if (ndRssi + rssi > my_rank.rssi)
        {
            PRINTF("better link quality\n");
            update_rank(ndRssi + rssi, ndRank, (linkaddr_t *)packetbuf_addr(PACKETBUF_ADDR_SENDER));
        }
    }
    else if (ndRank < my_rank.rank - 1)
    {
        update_rank(ndRssi + rssi, ndRank, (linkaddr_t *)packetbuf_addr(PACKETBUF_ADDR_SENDER));
    }
#endif
    sdn_ds_nbr_add((linkaddr_t *)packetbuf_addr(PACKETBUF_ADDR_SENDER), &ndRank, &ndRssi, &rssi, NULL);
}
/*---------------------------------------------------------------------------*/
static void send_nd_output(void)
{
    /* IP packet */
    SDN_IP_BUF->vahl = (0x01 << 5) | SDN_IPH_LEN;
    /* Total length */
    sdn_len = SDN_IPH_LEN + SDN_NDH_LEN;
    SDN_IP_BUF->len = sdn_len;

    SDN_IP_BUF->ttl = 0x40;

    SDN_IP_BUF->proto = SDN_PROTO_ND;

    SDN_IP_BUF->scr.u16 = sdnip_htons(linkaddr_node_addr.u16);

    SDN_IP_BUF->dest.u16 = sdnip_htons(linkaddr_null.u16);

    // memcpy(&SDN_IP_BUF->scr, &linkaddr_node_addr, sizeof(linkaddr_node_addr));
    // sdn_create_broadcast_addr(&SDN_IP_BUF->dest);
    // linkaddr_copy(&SDN_IP_BUF->dest, &linkaddr_null);

    SDN_IP_BUF->ipchksum = 0;
    SDN_IP_BUF->ipchksum = ~sdn_ipchksum();

    /* ND packet */
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
    SDN_ND_BUF->rank = sdnip_htons(my_rank.rank);
    SDN_ND_BUF->rssi = sdnip_htons(my_rank.rssi);
#else
    SDN_ND_BUF->rank = 0;
    SDN_ND_BUF->rssi = 0;
#endif

    SDN_ND_BUF->ndchksum = 0;
    SDN_ND_BUF->ndchksum = ~sdn_ndchksum();

    print_buff(sdn_buf, sdn_len, true);

    PRINTF("Sending ND packet (rank: %d, rssi: %d)\n",
           sdn_ntohs(SDN_ND_BUF->rank),
           sdn_ntohs(SDN_ND_BUF->rssi));

    /* Update statistics */
    SDN_STAT(++sdn_stat.ip.sent);
    SDN_STAT(++sdn_stat.nd.sent);
    SDN_STAT(sdn_stat.nd.sent_bytes += sdn_len);

    sdnbuf_set_attr(SDNBUF_ATTR_MAX_MAC_TRANSMISSIONS, 3);

    sdn_ip_output(NULL);
}
/*---------------------------------------------------------------------------*/
static void sdn_send_nd_periodic(void)
{
    send_nd_output();
    PRINTF("sending ND message.\n");
    rand_time = SDN_MIN_ND_INTERVAL + random_rand() %
                                          (uint16_t)(SDN_MAX_ND_INTERVAL - SDN_MIN_ND_INTERVAL);
    PRINTF("Random time 1 = %u\n", rand_time);
    stimer_set(&nd_timer_na, rand_time);
}
/*---------------------------------------------------------------------------*/
void sdn_nd_periodic(void)
{
    /* Periodic ND sending */
    // PRINTF("periodic.\n");
    if (stimer_expired(&nd_timer_na) /* && (sdn_len == 0) */)
    {
        sdn_send_nd_periodic();
    }
    etimer_reset(&nd_timer_periodic);
}