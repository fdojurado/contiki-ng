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
//process_start(&sdn_process, NULL);

/*-------------------------------TSCH configuration---------------------------*/
/* Put all cells on the same slotframe */
#define APP_SLOTFRAME_HANDLE 1
/* Put all unicast cells on the same timeslot (for demonstration purposes only) */
#define APP_UNICAST_TIMESLOT 1
static void
initialize_tsch_schedule(void)
{
    int i, j;
    printf("printing current schedule1\n");
    tsch_schedule_print();
    printf("end printing current schedule1\n");
    struct tsch_slotframe *sf_common = tsch_schedule_add_slotframe(APP_SLOTFRAME_HANDLE, APP_SLOTFRAME_SIZE);
    uint16_t slot_offset;
    uint16_t channel_offset;

    printf("printing current schedule2\n");
    tsch_schedule_print();
    printf("end printing current schedule2\n");

    /* A "catch-all" cell at (0, 0) */
    slot_offset = 0;
    channel_offset = 0;
    tsch_schedule_add_link(sf_common,
                           LINK_OPTION_RX | LINK_OPTION_TX | LINK_OPTION_SHARED,
                           LINK_TYPE_ADVERTISING, &tsch_broadcast_address,
                           slot_offset, channel_offset, 0);

    printf("printing current schedule3\n");
    tsch_schedule_print();
    printf("end printing current schedule3\n");
    for (i = 0; i < 5 - 1; ++i)
    {
        uint8_t link_options;
        linkaddr_t addr;
        uint16_t remote_id = i + 1;

        for (j = 0; j < sizeof(addr); j += 2)
        {
            addr.u8[j + 1] = remote_id & 0xff;
            addr.u8[j + 0] = remote_id >> 8;
            LOG_INFO("Setting addr");
            LOG_INFO_LLADDR((linkaddr_t *)&addr);
            LOG_INFO_("\n");
        }

        /* Add a unicast cell for each potential neighbor (in Cooja) */
        /* Use the same slot offset; the right link will be dynamically selected at runtime based on queue sizes */
        slot_offset = APP_UNICAST_TIMESLOT;
        channel_offset = i;
        /* Warning: LINK_OPTION_SHARED cannot be configured, as with this schedule
     * backoff windows will not be reset correctly! */
        link_options = remote_id == node_id ? LINK_OPTION_RX : LINK_OPTION_TX;

        tsch_schedule_add_link(sf_common,
                               link_options,
                               LINK_TYPE_NORMAL, &addr,
                               slot_offset, channel_offset, 1);
    }

    printf("printing current schedule4\n");
    tsch_schedule_print();
    printf("end printing current schedule4\n");
}
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

    PROCESS_BEGIN();

    etimer_set(&stats_timer, CLOCK_SECOND * 1);

    /* To check whether the node still alive or not. */
    etimer_set(&alive_timer, CLOCK_SECOND / 2);

#if FLOCKLAB_DEPLOYMENT
    linkaddr_t node_addr;
    node_id = FLOCKLAB_NODE_ID;
    node_addr.u8[0] = node_id;
    node_addr.u8[1] = node_id;
    linkaddr_set_node_addr(&node_addr);
#endif

    /* #if !FLOCKLAB_DEPLOYMENT
    node_addr.u8[0] = linkaddr_node_addr.u8[1];
    node_addr.u8[1] = linkaddr_node_addr.u8[1];
    linkaddr_set_node_addr(&node_addr);
#endif */
    /* set controller address assuming is a Z1 mote */
    ctrl_addr.u8[0] = 1;
    ctrl_addr.u8[1] = 0;

    LOG_INFO("Setting controller addr ");
    LOG_INFO_LLADDR((linkaddr_t *)&ctrl_addr);
    LOG_INFO_("\n");
    /* Initialize NullNet */
    //sdn_net_buf = (uint8_t *)&count;
    //sdn_net_len = sizeof(count);
    // sdn_net_set_input_callback(input_callback);

    // etimer_set(&periodic_timer, SEND_INTERVAL);
#if MAC_CONF_WITH_TSCH
    initialize_tsch_schedule();
    if (node_id == 1)
    { /* Running on the root? */
        LOG_INFO("Setting as the root\n");
        tsch_set_coordinator(1);
    }
#endif
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
