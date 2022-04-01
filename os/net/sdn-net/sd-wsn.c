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

#include "sd-wsn.h"
#include "sdnbuf.h"
#include "sdn-neighbor-discovery.h"
#include "sdn-advertisement.h"
#include "sdn-ds-nbr.h"
#include "net/ipv6/uip.h" // show this header be here?
#include "sdn-ds-route.h"
#include "sdn-network-config.h"
#include "sdn-data-packets.h"
#include "sdn-data-aggregation.h"
#if SDN_CONTROLLER
#include "sdn-controller/sdn-ds-node-route.h"
#include "sdn-controller/sdn-ds-config-routes.h"
#include "sdn-controller/sdn-ds-edge.h"
#include "sdn-controller/sdn-ds-node.h"
#include "sdn-controller/sdn-ds-dfs.h"
#endif
#if SERIAL_SDN_CONTROLLER
#include "sdn-controller-serial/sdn-serial.h"
#endif /* SERIAL_SDN_CONTROLLER */
/* Log configuration */
#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#if SDN_STATISTICS == 1
struct sdn_stats sdn_stat;
#endif

struct etimer sdn_ds_timer_periodic; /**< Timer for maintenance of data structures */

uint16_t sdn_len;
sdn_buf_t sdn_aligned_buf;

/*---------------------------------------------------------------------------*/
void sdn_wsn_init(void)
{
    sdnbuf_init();
    sdn_nd_init();
    sdn_ds_neighbors_init();
    sdn_ds_route_init();
    sdn_na_init();
    sdn_data_init();
    sdn_data_aggregation_init();
#if SDN_CONTROLLER
    sdn_ds_node_id_init();
    sdn_nc_init();
    sdn_ds_node_route_init();
    sdn_ds_edge_init();
    sdn_ds_config_routes_init();
    sdn_ds_node_init();
    dfs_init();
#endif
    etimer_set(&sdn_ds_timer_periodic, SDN_DS_PERIOD);
}
/*---------------------------------------------------------------------------*/
void print_buff(uint8_t *buf, size_t buflen, int8_t bare)
{
    // PRINTF("sof dump.\n");
    if (bare)
    {
        while (buflen--)
            PRINTF("%02X%s", *buf++, (buflen > 0) ? " " : "");
    }
    else
    {
        PRINTF("Dump: ");
        while (buflen--)
            PRINTF("%02X%s", *buf++, (buflen > 0) ? " " : "");
        PRINTF("\n");
    }
    PRINTF("\n");
}
/*---------------------------------------------------------------------------*/
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

    /* Return sum in host byte order. */
    return sum;
}
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
uint16_t
sdnip_htons(uint16_t val)
{
    return UIP_HTONS(val);
}
/*---------------------------------------------------------------------------*/
uint16_t
sdn_ipchksum(void)
{
    uint16_t sum;

    sum = chksum(0, sdn_buf, SDN_IPH_LEN);
    PRINTF("sdn_ipchksum: sum 0x%04x\n", sum);
    return (sum == 0) ? 0xffff : sdnip_htons(sum);
}
/*---------------------------------------------------------------------------*/
uint16_t sdn_ndchksum(void)
{
    uint16_t sum;

    sum = chksum(0, SDN_IP_PAYLOAD(0), SDN_NDH_LEN);
    PRINTF("sdn_ndchksum: sum 0x%04x\n", sum);
    return (sum == 0) ? 0xffff : sdnip_htons(sum);
}
/*---------------------------------------------------------------------------*/
uint16_t sdn_cpchksum(uint8_t len)
{
    uint16_t sum;

    sum = chksum(0, SDN_IP_PAYLOAD(0), SDN_CPH_LEN + len);
    PRINTF("sdn_cpchksum: sum 0x%04x\n", sum);
    return (sum == 0) ? 0xffff : sdnip_htons(sum);
}
/*---------------------------------------------------------------------------*/
void sdn_ds_periodic(void)
{
#if SDN_CONTROLLER
    /* Periodic processing on default routers */
    sdn_ds_node_periodic();
    sdn_ds_node_route_periodic();
#endif
    sdn_ds_neighbor_periodic();
    etimer_reset(&sdn_ds_timer_periodic);
}
/*---------------------------------------------------------------------------*/
static bool sdn_update_ttl(void)
{
    if (SDN_IP_BUF->ttl <= 1)
    {
        // uip_icmp6_error_output(ICMP6_TIME_EXCEEDED, ICMP6_TIME_EXCEED_TRANSIT, 0);
        SDN_STAT(++sdn_stat.ip.drop);
        return false;
    }
    else
    {
        SDN_IP_BUF->ttl = SDN_IP_BUF->ttl - 1;
        return true;
    }
}
/*---------------------------------------------------------------------------*/
void sdnip_process(uint8_t flag)
{
    uint8_t protocol;
    uint8_t *next_header;
    linkaddr_t dest;

#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER) || DEBUG
    linkaddr_t scr;
#endif

    print_buff(sdn_buf, sdn_len, true);

    /* This is where the input processing starts. */
    SDN_STAT(++sdn_stat.ip.recv);

    /* Compute IP layer checksum */
    if (sdn_ipchksum() != 0xffff)
    {
        SDN_STAT(++sdn_stat.ip.drop);
        SDN_STAT(++sdn_stat.ip.chkerr);
        PRINTF("ip bad checksum\n");
        goto drop;
    }

    /*
     * If the reported length in the ip header doesnot match the packet size,
     * then we drop the packet.
     */
    if (sdn_len < sdnbuf_get_len_field(SDN_IP_BUF))
    {
        SDN_STAT(++sdn_stat.ip.drop);
        PRINTF("packet shorter than reported in IP header\n");
        goto drop;
    }

    /* Check that the packet length is acceptable given our IP buffer size. */
    if (sdn_len > sizeof(sdn_buf))
    {
        SDN_STAT(++sdn_stat.ip.drop);
        PRINTF("dropping packet with length %d > %d\n",
               (int)sdn_len, (int)sizeof(sdn_buf));
        goto drop;
    }

    /*
     * Process Packets with a routable multicast destination:
     * - We invoke the multicast engine and let it do its thing
     *   (cache, forward etc).
     * - We never execute the datagram forwarding logic in this file here. When
     *   the engine returns, forwarding has been handled if and as required.
     * - Depending on the return value, we either discard or deliver up the stack
     *
     * All multicast engines must hook in here. After this function returns, we
     * expect UIP_BUF to be unmodified
     */

    dest.u16 = sdnip_htons(SDN_IP_BUF->dest.u16);

    if (!linkaddr_cmp(&dest, &linkaddr_node_addr) &&
        !linkaddr_cmp(&dest, &linkaddr_null))
    {
#if DEBUG
        scr.u16 = sdnip_htons(SDN_IP_BUF->scr.u16);
        PRINTF("sdn ip packet Not for us from %d.%d\n", scr.u8[0], scr.u8[1]);
#endif

#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
        /* Aggregate? */
        if (cluster_head &&
            SDN_IP_BUF->proto == SDN_PROTO_DATA &&
            ((SDN_IP_BUF->vahl >> 4) & 0x01))
        {
            sdn_seq_list_t hdr;
            // sdn_data_aggregation_t *rt;
            scr.u16 = sdnip_htons(SDN_DATA_BUF(0)->addr.u16);
            hdr.seq = sdnip_htons(SDN_DATA_BUF(0)->seq);
            hdr.temp = sdnip_htons(SDN_DATA_BUF(0)->temp);
            hdr.humidty = sdnip_htons(SDN_DATA_BUF(0)->humidty);
            PRINTF("aggregating packet: node %d.%d, seq %u temp %u hum %u\n",
                   scr.u8[0], scr.u8[1],
                   hdr.seq,
                   hdr.temp,
                   hdr.humidty);
            if (sdn_data_aggregation_add(&scr, &hdr) == NULL)
            {
                goto forward;
            }
            goto drop;
        }
    forward:
#endif
        if (!sdn_update_ttl())
        {
            PRINTF("ttl expired\n");
            SDN_STAT(++sdn_stat.nd.drop);
            goto drop;
        }
        PRINTF("Forwarding packet to destination %d.%d\n",
               dest.u8[0], dest.u8[1]);
        SDN_STAT(++sdn_stat.ip.forwarded);
        goto send;
    }

#if DEBUG
    scr.u16 = sdnip_htons(SDN_IP_BUF->scr.u16);
    PRINTF("sdn ip packet for us from %d.%d\n",
           scr.u8[1], scr.u8[0]);
#endif

    next_header = sdnbuf_get_next_header(sdn_buf, sdn_len, &protocol);

    /* Process upper-layer input */
    if (next_header != NULL)
    {
        switch (protocol)
        {
        case SDN_PROTO_ND:
            /* ND input */
            goto nd_input;
        case SDN_PROTO_CP:
            /* CP input */
            goto cp_input;
        case SDN_PROTO_DATA:
            /* Data input */
            goto data_input;
            break;
        }
        PRINTF("Protocol not found.\n");
        SDN_STAT(++sdn_stat.nd.drop);
        goto drop;
    }
nd_input:
    /* Compute and check the ICMP header checksum */

    if (sdn_ndchksum() != 0xffff)
    {
        SDN_STAT(++sdn_stat.nd.drop);
        SDN_STAT(++sdn_stat.nd.chkerr);
        PRINTF("nd bad checksum\n");
        goto drop;
    }
    /* This is ND processing. */
    sdn_nd_input();
    SDN_STAT(++sdn_stat.nd.recv);
    PRINTF("ND input length %d\n", sdn_len);
    goto drop;
data_input:
#if SDN_CONTROLLER
    /* Process incoming data packet */
    sdn_data_input();
    // SDN_STAT(++sdn_stat.data.recv);
    PRINTF("Data input length %d\n", sdn_len);
    goto drop;
#endif
cp_input:
    /* Compute control packet */
    next_header = cpbuf_get_next_header(next_header, sdn_len, &protocol);

    /* Process upper-layer input */
    if (next_header != NULL)
    {
        switch (protocol)
        {
        case SDN_PROTO_NA:
            /* ND input */
            goto na_input;
        case SDN_PROTO_NC:
            /* NC input */
            goto nc_input;
        case SDN_PROTO_NC_ACK:
            /* NC_ACK input */
            goto nc_ack_input;
        }
        PRINTF("Protocol not found.\n");
        goto drop;
    }
na_input:
#if SDN_CONTROLLER
    if (sdn_cpchksum(cpbuf_get_len_field(SDN_CP_BUF)) != 0xffff)
    {
        // SDN_STAT(++sdn_stat.nd.drop);
        // SDN_STAT(++sdn_stat.nd.chkerr);
        PRINTF("na bad checksum\n");
        goto drop;
    }
    /* This is NA processing. */
    sdn_na_input();
#endif
    goto drop;
nc_input:
#if !SDN_CONTROLLER || SERIAL_SDN_CONTROLLER
    if (sdn_cpchksum(cpbuf_get_len_field(SDN_CP_BUF)) != 0xffff)
    {
        // SDN_STAT(++sdn_stat.nd.drop);
        // SDN_STAT(++sdn_stat.nd.chkerr);
        PRINTF("nc bad checksum\n");
        goto drop;
    }
    /* This is NC processing. */
    sdn_nc_input();
    // At this stage, we have already built our ACK packet
    goto send;
#endif /* !SDN_CONTROLLER || SERIAL_SDN_CONTROLLER */
nc_ack_input:
#if SDN_CONTROLLER
    if (sdn_cpchksum(cpbuf_get_len_field(SDN_CP_BUF)) != 0xffff)
    {
        // SDN_STAT(++sdn_stat.nd.drop);
        // SDN_STAT(++sdn_stat.nd.chkerr);
        PRINTF("nc ack bad checksum\n");
        goto drop;
    }
#if SDN_CONTROLLER
    /* NC ack processing */
    sdn_nc_ack_input();
#endif
#endif
    goto drop;
send:
    /* Recalculate the checksum */
    SDN_IP_BUF->ipchksum = 0;
    SDN_IP_BUF->ipchksum = ~sdn_ipchksum();
    PRINTF("Forwarding packet with length %d (%d)\n", sdn_len, sdnbuf_get_len_field(SDN_IP_BUF));

    SDN_STAT(++sdn_stat.ip.sent);
    return;
drop:
    sdnbuf_clear();
    return;
}