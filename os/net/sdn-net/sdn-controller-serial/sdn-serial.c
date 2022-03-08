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
 *         SDN serial interface
 * \author
 *         Fernando Jurado <fdo.jurado@gmail.com>
 */

#include "sdn-serial.h"
#include "sdn-serial-protocol.h"
#include <string.h> /* for memcpy() */

#include "lib/ringbuf.h"

/* Log configuration */
#define DEBUG 1
#if DEBUG
#include <stdio.h>
#include "net/sdn-net/sd-wsn.h"
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#if (SDN_SERIAL_BUFSIZE & (SDN_SERIAL_BUFSIZE - 1)) != 0
#error SERIAL_LINE_CONF_BUFSIZE must be a power of two (i.e., 1, 2, 4, 8, 16, 32, 64, ...).
#error Change SERIAL_LINE_CONF_BUFSIZE in contiki-conf.h.
#endif

/* HDLC Asynchronous framing */
/* The frame boundary octet is 01111110, (7E in hexadecimal notation) */
#define FRAME_BOUNDARY_OCTET 0x7E

/* A "control escape octet", has the bit sequence '01111101', (7D hexadecimal) */
#define CONTROL_ESCAPE_OCTET 0x7D

/* If either of these two octets appears in the transmitted data, an escape octet is sent, */
/* followed by the original data octet with bit 5 inverted */
#define INVERT_OCTET 0x20

uint16_t sdn_serial_len;

static uint8_t rxbuf_data[SDN_SERIAL_BUFSIZE];
static uint8_t txbuf_data[SDN_SERIAL_BUFSIZE];
static struct ringbuf rxbuf, txbuf;

sdn_serial_send_char sdn_send_serial_function = NULL;

PROCESS(sdn_serial_process, "SDN serial driver");

process_event_t sdn_serial_ev;

/*---------------------------------------------------------------------------*/
#if DEBUG
static void sdn_serial_print_packet()
{
    /* Print header */
    PRINTF("---------SDN-SERIAL-PACKET--------------\n");
    PRINTF("addr: %d.%d\n", SDN_SERIAL_PACKET_BUF->addr.u8[0], SDN_SERIAL_PACKET_BUF->addr.u8[1]);
    PRINTF("type: %d\n", SDN_SERIAL_PACKET_BUF->type);
    PRINTF("payload lenght: %d\n", SDN_SERIAL_PACKET_BUF->payload_len);
    PRINTF("reserved 0: %d\n", SDN_SERIAL_PACKET_BUF->reserved[0]);
    PRINTF("reserved 1: %d\n", SDN_SERIAL_PACKET_BUF->reserved[1]);

    /* Print payload */
    print_buff(sdn_serial_packet_buf + SDN_SERIAL_PACKETH_LEN, SDN_SERIAL_PACKET_BUF->payload_len, 0);
    PRINTF("----------------------------------------\n");
}
#endif
/*---------------------------------------------------------------------------*/
/* Function to send a byte through Serial*/
static void sdn_serial_putchar(char data)
{
    if (sdn_send_serial_function)
    {
        (*sdn_send_serial_function)((int)data);
    }
}
/*---------------------------------------------------------------------------*/
static int copy_to_serial_buff()
{
    uint8_t data;
    /* Copy header */
    // get addr[0]
    data = ringbuf_get(&rxbuf);
    if (data != -1)
    {
        SDN_SERIAL_PACKET_BUF->addr.u8[0] = data;
    }
    else
    {
        PRINTF("error rx buffer empty");
        return 1; //error buffer empty
    }
    // get addr[1]
    data = ringbuf_get(&rxbuf);
    if (data != -1)
    {
        SDN_SERIAL_PACKET_BUF->addr.u8[1] = data;
    }
    else
    {
        PRINTF("error rx buffer empty");
        return 1; //error buffer empty
    }
    // type
    data = ringbuf_get(&rxbuf);
    if (data != -1)
    {
        SDN_SERIAL_PACKET_BUF->type = data;
    }
    else
    {
        PRINTF("error rx buffer empty");
        return 1; //error buffer empty
    }
    // payload len
    data = ringbuf_get(&rxbuf);
    if (data != -1)
    {
        SDN_SERIAL_PACKET_BUF->payload_len = data;
    }
    else
    {
        PRINTF("error rx buffer empty");
        return 1; //error buffer empty
    }
    // reserve[0]
    data = ringbuf_get(&rxbuf);
    if (data != -1)
    {
        SDN_SERIAL_PACKET_BUF->reserved[0] = data;
    }
    else
    {
        PRINTF("error rx buffer empty");
        return 1; //error buffer empty
    }
    // reserve[1]
    data = ringbuf_get(&rxbuf);
    if (data != -1)
    {
        SDN_SERIAL_PACKET_BUF->reserved[1] = data;
    }
    else
    {
        PRINTF("error rx buffer empty");
        return 1; //error buffer empty
    }

    /* Copy payload */
    uint16_t size = SDN_SERIAL_PACKET_BUF->payload_len; //payload size
    PRINTF("payload size %d\n", size);
    uint8_t i = 0;
    uint8_t *ptr;
    while (size)
    {
        data = ringbuf_get(&rxbuf);
        ptr = SDN_SERIAL_PACKET_PAYLOAD_BUF(i);
        if (data != -1)
        {
            *ptr = data;
        }
        else
        {
            PRINTF("error rx buffer empty");
            return 1; //error buffer empty
        }
        size--;
        i++;
    }

    sdn_serial_len = SDN_SERIAL_PACKETH_LEN + SDN_SERIAL_PACKET_BUF->payload_len;

    return 0;
}
/*---------------------------------------------------------------------------*/
static int copy_to_tx_buffer()
{
    /* Copy header */
    if (ringbuf_put(&txbuf, (uint8_t)SDN_SERIAL_PACKET_BUF->addr.u8[0]) == 0)
        return 1;
    if (ringbuf_put(&txbuf, (uint8_t)SDN_SERIAL_PACKET_BUF->addr.u8[1]) == 0)
        return 1;
    if (ringbuf_put(&txbuf, (uint8_t)SDN_SERIAL_PACKET_BUF->type) == 0)
        return 1;
    if (ringbuf_put(&txbuf, (uint8_t)SDN_SERIAL_PACKET_BUF->payload_len) == 0)
        return 1;
    if (ringbuf_put(&txbuf, (uint8_t)SDN_SERIAL_PACKET_BUF->reserved[0]) == 0)
        return 1;
    if (ringbuf_put(&txbuf, (uint8_t)SDN_SERIAL_PACKET_BUF->reserved[1]) == 0)
        return 1;

    /* Copy payload */
    uint16_t size = sdn_serial_len - SDN_SERIAL_PACKETH_LEN; //payload size
    // PRINTF("payload size %d\n", size);
    uint8_t i = 0;
    uint8_t *ptr;
    while (size)
    {
        ptr = SDN_SERIAL_PACKET_PAYLOAD_BUF(i);
        // PRINTF("in payload %d\n", *ptr);
        if (ringbuf_put(&txbuf, *ptr) == 0)
            return 1;
        size--;
        i++;
        // PRINTF("payload data %d\n", size);
    }
    return 0;
}
/*---------------------------------------------------------------------------*/
int sdn_serial_send(void)
{
    // Packet size
    uint16_t size = sdn_serial_len;
    uint8_t data = 0;
    PRINTF("size to send %d\n", size);

    /* copy SDN_SERIAL_BUF into outgoing txbuf_data  */
    if (copy_to_tx_buffer())
    {
        PRINTF("Fail copying to tx ring buffer\n");
        return 1; // Failed copying to buffer
    }

    sdn_serial_putchar((uint8_t)FRAME_BOUNDARY_OCTET);

    while (size)
    {
        data = ringbuf_get(&txbuf);
        if (data != -1)
        {
            if ((data == CONTROL_ESCAPE_OCTET) || (data == FRAME_BOUNDARY_OCTET))
            {
                sdn_serial_putchar((uint8_t)CONTROL_ESCAPE_OCTET);
                data ^= INVERT_OCTET;
            }
            // PRINTF("in tx buffer %x\n", data);
            sdn_serial_putchar((uint8_t)data);
            size--;
        }
        else
        {
            // no bytes left in buffer
            PRINTF("Buffer empty\n");
        }
    }
    sdn_serial_putchar((uint8_t)FRAME_BOUNDARY_OCTET);

    return 0;
}
/*---------------------------------------------------------------------------*/
int sdn_serial_input_byte(unsigned char data)
{
    static uint8_t overflow = 0; /* Buffer overflow: ignore until END */
    static uint8_t escape_character = 0;
    static int frame_length = 0;

    /* FRAME FLAG */
    if (data == FRAME_BOUNDARY_OCTET)
    {
        if (escape_character == 1)
        {
            PRINTF("serial: escape_character == true\n");
            escape_character = 0;
        }
        else if (overflow)
        { /* We lost consistence, begin again */
            /* Clear Buffer */
            PRINTF("serial overflow\n");
            overflow = 0;
            frame_length = 0;
        } /* If a valid frame is detected> FRAME_BOUNDARY + PACKET MINIMUM SIZE (HEADER), otherwise discard. */
        else if (frame_length >= SDN_SERIAL_PACKETH_LEN)
        {
            /* Wake up consumer process */
            frame_length = 0;
            PRINTF("Call process\n");
            process_post(&sdn_serial_process, sdn_serial_ev, NULL);
            return 0;
        }
        else
        {
            /* re-synchronization. Start over*/
            PRINTF("serial re-synchronization\n");
            frame_length = 0;
            return 1;
        }
        return 0;
    }
    if (escape_character)
    {

        escape_character = 0;
        data ^= INVERT_OCTET;
    }
    else if (data == CONTROL_ESCAPE_OCTET)
    {

        escape_character = 1;
        return 0;
    }

    if (frame_length < (SDN_SERIAL_MAX_PACKET_SIZE + 2))
    { // Adding 2 bytes from serial communication
        /* copy SDN_SERIAL_BUF into outgoing txbuf_data  */
        // PRINTF("data rcv:0x%X\n", data);
        if (ringbuf_put(&rxbuf, (uint8_t)data) == 0)
            return 1;
        // currentSerialPacket[frame_length] = data;
        frame_length++;
    }
    else
    {
        overflow = 1;
        PRINTF("Packet size overflow: %u bytes\n", frame_length);
    }
    return 0;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sdn_serial_process, ev, data)
{
    PROCESS_BEGIN();

    sdn_serial_ev = process_alloc_event();

    while (1)
    {
        PROCESS_WAIT_EVENT_UNTIL(ev == sdn_serial_ev);

        PRINTF("new packet\n");
        /* Copy rx ring buffer to SDN_SERIAL_PACKET_BUF */
        if (!copy_to_serial_buff())
        {
            // packet = (sdn_serial_packet_t *)data;
#if DEBUG
            sdn_serial_print_packet();
#endif
            process_post(&sdn_serial_protocol_process, SERIAL_PACKET_INPUT, NULL);

            /* Wait until all processes have handled the serial event */
            if (PROCESS_ERR_OK == process_post(PROCESS_CURRENT(), PROCESS_EVENT_CONTINUE, NULL))
            {
                PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);
            }
        }
    }

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void sdn_serial_init(sdn_serial_send_char callback)
{
    ringbuf_init(&rxbuf, rxbuf_data, sizeof(rxbuf_data));
    ringbuf_init(&txbuf, txbuf_data, sizeof(txbuf_data));
    sdn_send_serial_function = callback;
    PRINTF("Initialising sdn-serial.\n");
    process_start(&sdn_serial_process, NULL);
}
/*---------------------------------------------------------------------------*/
uint8_t sdn_serial_get_len(struct sdn_serial_packet_hdr *hdr)
{
    return hdr->payload_len + SDN_SERIAL_PACKETH_LEN;
}
/*---------------------------------------------------------------------------*/