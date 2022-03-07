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
 *         SDN serial protocol
 * \author
 *         Fernando Jurado <fdo.jurado@gmail.com>
 */

#include "sdn-serial-protocol.h"
#include "net/sdn-net/sdn-net.h"
#include "sdn-serial.h"
#include "sd-wsn.h"
#include "sdn.h"
#if !CONTIKI_TARGET_COOJA
#include "dev/uart1.h"
#endif /* !CONTIKI_TARGET_COOJA */

#ifdef Z1_DEF_H_
#include "dev/uart0.h"
#endif /* Z1_DEF_H_ */

#include <stdio.h>
#include <string.h> // needed for memcpy

/* Log configuration */
#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

sdn_serial_packet_buf_t sdn_serial_aligned_buf;

PROCESS(sdn_serial_protocol_process, "SDN serial protocol");

/*---------------------------------------------------------------------------*/
uint8_t serial_packet_output(void)
{
    if (sdn_serial_len == 0)
    {
        return 1;
    }
    if (!sdn_serial_send())
    {
        PRINTF("output: send %u bytes through serial\n", sdn_serial_len);
        return 0;
    }
    else
    {
        /* Ok, ignore and drop... */
        sdn_serial_len = 0;
        return 1;
    }
}
/*---------------------------------------------------------------------------*/
static void serial_packet_input(void)
{
    if (sdn_serial_len > 0)
    {
        PRINTF("input_serial: received %u bytes\n", sdn_serial_len);
        /* Inspect the received serial packet */
        // As this is a serial interface, we assume this is a reliable connection;
        // Therefore, we don't include a checksum field in the header of the serial pkt
        /*
         * If the reported length in the serial header doesnot match the packet size,
         * then we drop the packet.
         */
        if (sdn_serial_len < sdn_serial_get_len(SDN_SERIAL_PACKET_BUF))
        {
            // SDN_STAT(++sdn_stat.ip.drop); // no stats for now
            PRINTF(" serial line packet shorter than reported in the packet header\n");
            goto drop;
        }
        /* Check that the serial packet length is acceptable given our IP buffer size. */
        if (sdn_serial_len > sizeof(sdn_buf))
        {
            // SDN_STAT(++sdn_stat.ip.drop); // no stats for now
            PRINTF("dropping serial line packet with length %d > %d\n",
                   (int)sdn_serial_len, (int)sizeof(sdn_buf));
            goto drop;
        }
        /*
         * Here, we assume that we are receiveing Network Configuration (NC) packets
         * (do we expect any other type of packets? not for now) from the serial controller
         */
        /* We need to copy the serial data to the IP/layer 3 packet first */
        // Payload size - is this always the address to configure?
        uint8_t *buffer;
        buffer = (uint8_t *)SDN_IP_BUF;
        // Set first the payload before the chksum. Copy the NC packet in the payload of the CP packet
        memcpy(buffer, sdn_serial_packet_buf + SDN_SERIAL_PACKETH_LEN, SDN_SERIAL_PACKET_BUF->payload_len);
        sdn_len = SDN_SERIAL_PACKET_BUF->payload_len;
        sdn_input();
        if (sdn_len > 0)
        {
            sdn_output();
            return;
        }
    drop:
        PRINTF("Dropping serial line packet.\n");
    }
}
/*---------------------------------------------------------------------------*/
static void eventhandler(process_event_t ev, process_data_t data)
{
    switch (ev)
    {
    case SERIAL_PACKET_INPUT:
        PRINTF("New serial packet received.\n");
        serial_packet_input();
        break;

    default:
        break;
    }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sdn_serial_protocol_process, ev, data)
{
    PROCESS_BEGIN();

    /* This is platform dependent */
#ifdef Z1_DEF_H_
    uart0_set_input(&sdn_serial_input_byte);
#endif /* Z1_DEF_H_ */
#ifdef CC26XX_UART_H_
    uart1_init(BAUD2UBR(115200));            // set the baud rate as necessary
    uart1_set_input(&sdn_serial_input_byte); // set the callback function
#endif
#ifdef UART0_ARCH_H_
    uart0_init(BAUD2UBR(115200));               // set the baud rate as necessary
    uart0_set_callback(&sdn_serial_input_byte); // set the callback function
#endif

    /* Send a packet */
    /* SDN_SERIAL_PACKET_BUF->addr.u8[0] = 0x01;
    SDN_SERIAL_PACKET_BUF->addr.u8[1] = 0x7E;
    SDN_SERIAL_PACKET_BUF->type = SDN_SERIAL_MSG_TYPE_EMPTY;
    SDN_SERIAL_PACKET_BUF->payload_len = SDN_SERIAL_PACKETH_LEN;
    struct sdn_serial_packet_hdr *ptr;
    ptr = (struct sdn_serial_packet_hdr *)SDN_SERIAL_PACKET_PAYLOAD_BUF(0);
    ptr->addr.u8[0] = 3;
    ptr->addr.u8[1] = 4;
    ptr->type = 2;
    ptr->payload_len = 0;
    ptr->reserved[0] = 9;
    ptr->reserved[1] = 15;
    sdn_serial_send(); */

    while (1)
    {
        PROCESS_YIELD();
        eventhandler(ev, data);
    }

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void sdn_serial_protocol_init(void)
{
    PRINTF("Initialising sdn-serial protocol.\n");
    sdn_serial_init(&putchar);
    process_start(&sdn_serial_protocol_process, NULL);
}