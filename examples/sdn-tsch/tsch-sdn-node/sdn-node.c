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
#include "net/sdn-net/sdn.h"
#include "net/sdn-net/sd-wsn.h"
#include "net/sdn-net/sdn-net.h"
#include "net/mac/tsch/tsch.h"
#include "lib/random.h"
#include "sys/node-id.h"
#include <string.h>
#include <stdio.h> /* For printf() */
#include "services/sdn-energy/sdn-energy.h"

#if MAC_CONF_WITH_TSCH
#include "net/mac/tsch/tsch.h"
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

/* Configuration */
// #define SEND_INTERVAL (8 * CLOCK_SECOND)

/*---------------------------------------------------------------------------*/
PROCESS(sdn_node_process, "sdn-node example");
// AUTOSTART_PROCESSES(&sdn_node_process, &sdn_process, &sdn_energy);
AUTOSTART_PROCESSES(&sdn_node_process, &sdn_energy);
// AUTOSTART_PROCESSES(&sdn_process);
// process_start(&sdn_process, NULL);

/*-------------------------------TSCH configuration---------------------------*/
/*---------------------------------------------------------------------------*/
void static print_stats(void)
{
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
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sdn_node_process, ev, data)
{
    static struct etimer stats_timer, alive_timer;
    int is_coordinator;

    PROCESS_BEGIN();

    etimer_set(&stats_timer, CLOCK_SECOND * 1);

    /* To check whether the node still alive or not. */
    etimer_set(&alive_timer, CLOCK_SECOND / 2);

#if CONTIKI_TARGET_IOTLAB
    // node id for the grenoble m-3-1 is 9044 in decimal, or 0x2354
    // The controller address is obtained adding one to the address of the sink
    ctrl_addr.u8[0] = 0x55;
    ctrl_addr.u8[1] = 0x23;
#else
    /* set controller address assuming is a Z1 mote */
    ctrl_addr.u8[0] = 1;
    ctrl_addr.u8[1] = 1;
#endif /* CONTIKI_TARGET_IOTLAB */

    LOG_INFO("Setting controller addr ");
    LOG_INFO_LLADDR((linkaddr_t *)&ctrl_addr);
    LOG_INFO_("\n");
    /* Initialize NullNet */
    // sdn_net_buf = (uint8_t *)&count;
    // sdn_net_len = sizeof(count);
    //  sdn_net_set_input_callback(input_callback);

    // etimer_set(&periodic_timer, SEND_INTERVAL);
    is_coordinator = 0;

#if CONTIKI_TARGET_COOJA || CONTIKI_TARGET_Z1
    is_coordinator = (node_id == 1);
#endif
#if CONTIKI_TARGET_IOTLAB
    // node id for the grenoble m-3-1 is 9044 in decimal, or 0x2354
    is_coordinator = (node_id == 9044);
#endif

    if (is_coordinator)
    {
        LOG_INFO("Setting as the root\n");
        tsch_set_coordinator(1);
    }
    NETSTACK_MAC.on();
    process_start(&sdn_process, NULL);
    while (1)
    {
        PROCESS_YIELD();
        if (ev == PROCESS_EVENT_TIMER && data == &alive_timer)
        {
            if (energy < 100)
            {
                /* This node is dead */
                // LOG_INFO("Node dead\n");
                SDN_STAT(++sdn_stat.nodes.dead);
                print_stats();
                /* Turn off the radio */
                NETSTACK_MAC.off();
                NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, -25);
                process_exit(&sdn_process);
                process_exit(&sdn_energy);
                PROCESS_EXIT();
            }
            etimer_reset(&alive_timer);
        }
        // if (ev == PROCESS_EVENT_TIMER && data == &stats_timer)
        // {
        //     printf("printing current schedule\n");
        //     tsch_schedule_print();
        //     printf("end printing current schedule\n");
        //     etimer_reset(&stats_timer);
        // }
    }

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
