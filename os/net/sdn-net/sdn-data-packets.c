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
// #include "sdn-data-aggregation.h"
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

#if BUILD_WITH_SDN_CONTROLLER_SERIAL
#include "sdn-controller-serial/sdn-serial.h"
#include "sdn-controller-serial/sdn-serial-protocol.h"
#endif /* BUILD_WITH_SDN_CONTROLLER_SERIAL */

#if BUILD_WITH_SDN_ORCHESTRA
#include "net/mac/tsch/tsch.h"
#endif /* BUILD_WITH_SDN_ORCHESTRA */

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "DATA"
#if LOG_CONF_LEVEL_DATA
#define LOG_LEVEL LOG_CONF_LEVEL_DATA
#else
#define LOG_LEVEL LOG_LEVEL_NONE
#endif /* LOG_CONF_LEVEL_DATA */

/** Period for uip-ds6 periodic task*/
#ifndef SDN_DATA_CONF_PERIOD
#define SDN_DATA_PERIOD (CLOCK_SECOND / 10)
#else
#define SDN_DATA_PERIOD SDN_DATA_CONF_PERIOD
#endif

#if !(SDN_CONTROLLER || BUILD_WITH_SDN_CONTROLLER_SERIAL)
struct etimer data_timer_periodic;
struct stimer data_timer_send; /**< ND timer, to schedule ND sending */
static uint16_t rand_time;     /**< random time value for timers */
static uint16_t cycle_seq = 0;
static uint8_t seq = 0;
#endif

/*---------------------------------------------------------------------------*/
void sdn_data_init(void)
{
#if !BUILD_WITH_SDN_CONTROLLER_SERIAL
    etimer_set(&data_timer_periodic, SDN_DATA_PERIOD);
    stimer_set(&data_timer_send, 2); /* wait to have a link local IP address */
#endif
    return;
}
/*---------------------------------------------------------------------------*/
#if !BUILD_WITH_SDN_CONTROLLER_SERIAL
void sdn_data_reset_seq(uint16_t new_cycle_seq)
{
    cycle_seq = new_cycle_seq;
    seq = 0;
}
#endif
/*---------------------------------------------------------------------------*/
#if !BUILD_WITH_SDN_CONTROLLER_SERIAL
static void send_data_output(void)
{
    const linkaddr_t *nxthop;
    nxthop = NETSTACK_ROUTING.nexthop(&ctrl_addr);
    if (nxthop != NULL)
    {
        SDN_IP_BUF->vap = (0x01 << 5) | SDN_PROTO_DATA;

        sdn_len = SDN_IPH_LEN + SDN_DATA_LEN;

        SDN_IP_BUF->tlen = sdn_len;

        SDN_IP_BUF->ttl = 0x40;

        SDN_IP_BUF->scr.u16 = sdnip_htons(linkaddr_node_addr.u16);

        SDN_IP_BUF->dest.u16 = sdnip_htons(ctrl_addr.u16);

        /* Data packet */
        seq++;
        SDN_DATA_BUF->cycle_seq = sdnip_htons(cycle_seq);
        SDN_DATA_BUF->seq = seq;
        SDN_DATA_BUF->temp = random_rand() % (uint8_t)(0x23);
        SDN_DATA_BUF->humidity = random_rand() % (uint8_t)(0x64);
        SDN_DATA_BUF->light = random_rand() % (uint8_t)(0x64);

#if BUILD_WITH_SDN_ORCHESTRA
        // LOG_INFO("Current ASN %lu.\n", tsch_current_asn.ls4b);
        SDN_DATA_BUF->asn_ls2b = sdnip_htons(tsch_current_asn.ls4b & 0x0000FFFF);
        SDN_DATA_BUF->asn_ms2b = sdnip_htons(tsch_current_asn.ls4b & 0xFFFF0000);
#else
        SDN_DATA_BUF->asn.ls2b = 0;
        SDN_DATA_BUF->asn.ms2b = 0;
#endif /* BUILD_WITH_SDN_ORCHESTRA */

        SDN_IP_BUF->hdr_chksum = 0;
        SDN_IP_BUF->hdr_chksum = ~sdn_ipchksum();

        /* Update statistics */
        SDN_STAT(++sdn_stat.ip.sent);
        // if (!aggregate)
        // {
        SDN_STAT(++sdn_stat.data.sent_nagg);
        SDN_STAT(sdn_stat.data.sent_nagg_bytes += sdn_len);
        // }
        // else
        // {
        //     SDN_STAT(++sdn_stat.data.sent_agg);
        //     SDN_STAT(sdn_stat.data.sent_agg_bytes += sdn_len);
        // }
        LOG_INFO("Sending Data pkt (SEQ: %d).\n", seq);

        print_buff(sdn_buf, sdn_len, true);

        // sdnbuf_set_attr(SDNBUF_ATTR_MAX_MAC_TRANSMISSIONS, 3);

        sdn_ip_output(nxthop);

        // sdn_data_aggregation_flush_all();
    }
}
#endif
/*---------------------------------------------------------------------------*/
#if !BUILD_WITH_SDN_CONTROLLER_SERIAL
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
#if !BUILD_WITH_SDN_CONTROLLER_SERIAL
    if (stimer_expired(&data_timer_send) /* && (sdn_len == 0) */)
    {
        sdn_send_nd_periodic();
    }
    etimer_reset(&data_timer_periodic);
#endif
}