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
#include "sdn-serial-protocol.h"
#include "net/netstack.h"
#include "net/sdn-net/sd-wsn.h"
#include "net/sdn-net/sdn-net.h"
#include "net/mac/tsch/tsch.h"
#include "net/sdn-net/sdn.h"
#include "sdn-serial.h"
#include "sys/node-id.h"
#include <string.h>
#include <stdio.h> /* For printf() */

#if CONTIKI_TARGET_IOTLAB
#include "platform.h"
#endif /* CONTIKI_TARGET_IOTLAB */

#if CONTIKI_TARGET_WISMOTE
#include "dev/cc2520/cc2520.h"
#endif

/* Log configuration */
#define DEBUG 1
#if DEBUG
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

linkaddr_t ctrl_addr;

#if FLOCKLAB_DEPLOYMENT
uint16_t FLOCKLAB_NODE_ID = 0xbeef; // any value is ok, will be overwritten by FlockLab
volatile uint16_t node_id;          // must be volatile
#endif

/* Configuration */
// #define SEND_INTERVAL (8 * CLOCK_SECOND)

/*---------------------------------------------------------------------------*/
PROCESS(serial_sdn_controller_process, "serial-sdn-controller example");
// AUTOSTART_PROCESSES(&serial_sdn_controller_process, &sdn_process);
AUTOSTART_PROCESSES(&serial_sdn_controller_process);
// process_start(&sdn_process, NULL);
/*-------------------------------TSCH configuration---------------------------*/
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(serial_sdn_controller_process, ev, data)
{
    // static struct etimer stats_timer;
    int is_coordinator;

    PROCESS_BEGIN();

    // etimer_set(&stats_timer, CLOCK_SECOND * 10);

    // Set the controller address as 1.1 where the sink takes the 1.0 address
    ctrl_addr.u8[0] = 1;
    ctrl_addr.u8[1] = 1;

    is_coordinator = 0;

#if BUILD_WITH_SDN_CONTROLLER_SERIAL || SDN_CONTROLLER
    is_coordinator = 1;
#endif

    if (is_coordinator)
    {
        LOG_INFO("Setting as the root\n");
        tsch_set_coordinator(1);
    }
    NETSTACK_MAC.on();

#if CONTIKI_TARGET_WISMOTE
    cc2520_set_txpower(0xF7);
#endif

#if CONTIKI_TARGET_IOTLAB
    // Possible values for M3 radio 3, 2.8, 2.3, 1.8, 1.3, 0.7, 0.0, -1,
    // -2, -3, -4, -5, -7, -9, -12, -17
    // see phy.h for correct value to use
    NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, PHY_POWER_0dBm);
#endif /* CONTIKI_TARGET_IOTLAB */

    NETSTACK_MAC.on();

    process_start(&sdn_process, NULL);
    /* start the sdn serial interface */
    sdn_serial_protocol_init();

    /* Initialize NullNet */
    // sdn_net_buf = (uint8_t *)&count;
    // sdn_net_len = sizeof(count);
    //  sdn_net_set_input_callback(input_callback);

    // etimer_set(&periodic_timer, SEND_INTERVAL);
    while (1)
    {
        PROCESS_YIELD();
        // if (ev == PROCESS_EVENT_TIMER && data == &stats_timer)
        // {
        //     printf("3, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu\n",
        //            SDN_STAT(sdn_stat.ip.forwarded),
        //            SDN_STAT(sdn_stat.data.sent_agg),
        //            SDN_STAT(sdn_stat.data.sent_agg_bytes),
        //            SDN_STAT(sdn_stat.data.sent_nagg),
        //            SDN_STAT(sdn_stat.data.sent_nagg_bytes),
        //            SDN_STAT(sdn_stat.cp.adv),
        //            SDN_STAT(sdn_stat.cp.adv_bytes),
        //            SDN_STAT(sdn_stat.cp.nc),
        //            SDN_STAT(sdn_stat.cp.nc_bytes),
        //            SDN_STAT(sdn_stat.nd.sent),
        //            SDN_STAT(sdn_stat.nd.sent_bytes),
        //            SDN_STAT(sdn_stat.nodes.dead));
        //     etimer_reset(&stats_timer);
        // }
        // if (ev == PROCESS_EVENT_TIMER && data == &et)
        // {
        //     printf("printing current schedule\n");
        //     tsch_schedule_print();
        //     printf("end printing current schedule\n");
        //     etimer_reset(&et);
        // }
    }

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
