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
        /* We need to copy the serial data to the IP packet first */
        // Payload size
        int8_t payload_size = SDN_SERIAL_PACKET_BUF->payload_len;
        // Version, aggregation flag, and header lenght
        SDN_IP_BUF->vahl = (0x01 << 5) | SDN_IPH_LEN;
        // Total length
        sdn_len = SDN_IPH_LEN + SDN_CPH_LEN + payload_size;
        // Set lenght in the IP buffer
        SDN_IP_BUF->len = sdn_len;
        // Set time to live
        SDN_IP_BUF->ttl = 0x40;
        // Set upper layer protocol
        SDN_IP_BUF->proto = SDN_PROTO_CP;
        // Set source address
        SDN_IP_BUF->scr.u16 = sdnip_htons(linkaddr_node_addr.u16);
        // Set destination address
        SDN_IP_BUF->dest.u16 = sdnip_htons(ctrl_addr.u16);
        sdn_input();
        if (sdn_serial_len > 0)
        {
            sdn_output();
        }
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