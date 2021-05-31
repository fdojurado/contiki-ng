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
 *         SDN-NET, a network layer for SD-WSN.
 * \author
 *         Fernando Jurado <fdo.jurado@gmail.com>
 *
 */

#include "contiki.h"
#include "net/netstack.h"
#include "net/sdn-net/sd-wsn.h"
#include "net/sdn-net/sdn-net.h"
#include "net/sdn-net/sdn.h"
#include <string.h>
#include <stdio.h> /* For printf() */

#if CONTIKI_TARGET_WISMOTE
#include "dev/cc2520/cc2520.h"
#endif

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

linkaddr_t ctrl_addr;

#if FLOCKLAB_DEPLOYMENT
uint16_t FLOCKLAB_NODE_ID = 0xbeef; // any value is ok, will be overwritten by FlockLab
volatile uint16_t node_id;          // must be volatile
#endif

#if MAC_CONF_WITH_TSCH
#include "net/mac/tsch/tsch.h"
#endif /* MAC_CONF_WITH_TSCH */

/* Configuration */
// #define SEND_INTERVAL (8 * CLOCK_SECOND)

/*---------------------------------------------------------------------------*/
PROCESS(sdn_controller_process, "sdn-controller example");
// AUTOSTART_PROCESSES(&sdn_controller_process, &sdn_process);
AUTOSTART_PROCESSES(&sdn_controller_process);
//process_start(&sdn_process, NULL);

/*---------------------------------------------------------------------------*/
// void input_callback(const void *data, uint16_t len,
//                     const linkaddr_t *src, const linkaddr_t *dest)
// {
//     if (len == sizeof(unsigned))
//     {
//         unsigned count;
//         memcpy(&count, data, sizeof(count));
//         PRINTF("Received %u from ", count);
//         PRINTF_LLADDR(src);
//         PRINTF_("\n");
//     }
// }
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sdn_controller_process, ev, data)
{
    static struct etimer stats_timer;

    PROCESS_BEGIN();

    etimer_set(&stats_timer, CLOCK_SECOND * 10);

#if FLOCKLAB_DEPLOYMENT
    node_id = FLOCKLAB_NODE_ID;
    ctrl_addr.u8[0] = node_id;
    ctrl_addr.u8[1] = node_id;
    linkaddr_set_node_addr(&ctrl_addr);
#endif

    linkaddr_copy(&ctrl_addr, &linkaddr_node_addr);

#if MAC_CONF_WITH_TSCH
    static struct etimer et;
    etimer_set(&et, CLOCK_SECOND);
    tsch_set_coordinator(1);
    while (tsch_is_associated == 0)
    {
        PROCESS_YIELD_UNTIL(etimer_expired(&et));
        etimer_reset(&et);
    }
    LOG_INFO("tsch_is_associated\n");
#endif /* MAC_CONF_WITH_TSCH */

#if CONTIKI_TARGET_WISMOTE
    cc2520_set_txpower(0xF7);
#endif

    NETSTACK_MAC.on();
    
    process_start(&sdn_process, NULL);
    /* Initialize NullNet */
    //sdn_net_buf = (uint8_t *)&count;
    //sdn_net_len = sizeof(count);
    // sdn_net_set_input_callback(input_callback);

    //etimer_set(&periodic_timer, SEND_INTERVAL);
    while (1)
    {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&stats_timer));
        LOG_INFO("3, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu\n",
               SDN_STAT(sdn_stat.ip.forwarded),
               SDN_STAT(sdn_stat.data.sent_agg),
               SDN_STAT(sdn_stat.data.sent_agg_bytes),
               SDN_STAT(sdn_stat.data.sent_nagg),
               SDN_STAT(sdn_stat.data.sent_nagg_bytes),
               SDN_STAT(sdn_stat.cp.adv),
               SDN_STAT(sdn_stat.cp.adv_bytes),
               SDN_STAT(sdn_stat.cp.nc),
               SDN_STAT(sdn_stat.cp.nc_bytes),
               SDN_STAT(sdn_stat.nd.sent),
               SDN_STAT(sdn_stat.nd.sent_bytes),
               SDN_STAT(sdn_stat.nodes.dead));
        etimer_reset(&stats_timer);
    }

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
