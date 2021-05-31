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

/**
 * \addtogroup sdn-net
 * @{
 */

#include "contiki.h"
#include "net/packetbuf.h"
#include "net/netstack.h"
#include "sdn-net.h"
#include "sdn.h"
#include "sdnbuf.h"
#include "sdn-neighbor-discovery.h"
#include "sd-wsn.h"
#include "sdn-advertisement.h"
#include "sdn-ds-nbr.h"
#include "sdn-ds-route.h"
#include "sdn-network-config.h"
#include "dev/watchdog.h"
#include <string.h>
/* Log configuration */
/* Log configuration */
#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

// uint8_t *sdn_net_buf;
// uint16_t sdn_net_len;
// static sdn_net_input_callback current_callback = NULL;
/**
 * A pointer to the packetbuf buffer.
 * We initialize it to the beginning of the packetbuf buffer, then
 * access different fields by updating the offset packetbuf_hdr_len.
 */
static uint8_t *packetbuf_ptr;
/**
 * packetbuf_hdr_len is the total length of (the processed) 6lowpan headers
 * (fragment headers, IPV6 or HC1, HC2, and HC1 and HC2 non compressed
 * fields).
 */
static uint8_t packetbuf_hdr_len;

/**
 * The length of the payload in the Packetbuf buffer.
 * The payload is what comes after the compressed or uncompressed
 * headers (can be the IP payload if the IP header only is compressed
 * or the UDP payload if the UDP header is also compressed)
 */
// static int packetbuf_payload_len;

static int last_rssi;
/*--------------------------------------------------------------------*/
static void
init(void)
{
  PRINTF("init\n");
  // current_callback = NULL;
}
/*--------------------------------------------------------------------*/
static void
input(void)
{

  uint8_t *buffer;

  /* init */
  packetbuf_hdr_len = 0;

  /* The MAC puts the 15.4 payload inside the packetbuf data buffer */
  packetbuf_ptr = packetbuf_dataptr();

  if (packetbuf_datalen() == 0)
  {
    PRINTF("input: empty packet\n");
    return;
  }

  /* Clear uipbuf and set default attributes */
  sdnbuf_clear();

  /* This is default sdn_buf since we assume that this is not fragmented */
  buffer = (uint8_t *)SDN_IP_BUF;

  /* Save the RSSI of the incoming packet in case the upper layer will
     want to query us for it later. */
  last_rssi = (signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI);

  sdn_len = packetbuf_datalen();

  /* Put uncompressed IP header in sicslowpan_buf. */
  memcpy(buffer, packetbuf_ptr, sdn_len);

  // if (current_callback != NULL)
  // {
  PRINTF("received %u bytes from %d.%d\n", packetbuf_datalen(),
         packetbuf_addr(PACKETBUF_ADDR_SENDER)->u8[0],
         packetbuf_addr(PACKETBUF_ADDR_SENDER)->u8[1]);
  // current_callback(packetbuf_dataptr(), packetbuf_datalen(),
  //                  packetbuf_addr(PACKETBUF_ADDR_SENDER), packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
  // }
  sdn_ip_input();
}
/*--------------------------------------------------------------------*/
// void sdn_net_set_input_callback(sdn_net_input_callback callback)
// {
//   current_callback = callback;
// }
/*--------------------------------------------------------------------*/
static uint8_t
output(const linkaddr_t *localdest)
{

  /* The MAC address of the destination of the packet */
  /* init */
  // packetbuf_hdr_len = 0;
  /* The MAC address of the destination of the packet */
  linkaddr_t dest;
  /* reset packetbuf buffer */
  // packetbuf_clear();
  // packetbuf_ptr = packetbuf_dataptr();
  // packetbuf_payload_len = 0;
  /*
   * The destination address will be tagged to each outbound
   * packet. If the argument localdest is NULL, we are sending a
   * broadcast packet.
   */
  packetbuf_clear();
  packetbuf_copyfrom(sdn_buf, sdn_len);

  /*
   * The destination address will be tagged to each outbound
   * packet. If the argument localdest is NULL, we are sending a
   * broadcast packet.
   */
  if (localdest == NULL)
  {
    linkaddr_copy(&dest, &linkaddr_null);
  }
  else
  {
    linkaddr_copy(&dest, localdest);
  }
  // if (dest == NULL)
  // {
  //   PRINTF("address null\n");
  //   // linkaddr_copy(&dest, &linkaddr_null);
  //   packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &linkaddr_null);
  // }
  // else
  // {
  //   PRINTF("address not null\n");
  //   // linkaddr_copy(&dest, localdest);
  //   packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, dest);
  // }
  packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &dest);
  packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &linkaddr_node_addr);
  /* copy over the retransmission count from uipbuf attributes */
  packetbuf_set_attr(PACKETBUF_ATTR_MAX_MAC_TRANSMISSIONS,
                     sdnbuf_get_attr(SDNBUF_ATTR_MAX_MAC_TRANSMISSIONS));
  // packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &dest);

  // PRINTF("output: sending SDN IP packet with len %d to %d.%d\n", sdn_len,
  //          dest.u8[0], dest.u8[1]);

  PRINTF("sending %u bytes to %d.%d\n", packetbuf_datalen(),
         packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[0],
         packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[1]);

  // packetbuf_clear();
  // packetbuf_copyfrom(sdn_net_buf, sdn_net_len);
  // if(dest != NULL) {
  //   packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, dest);
  // } else {
  //   packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &linkaddr_null);
  // }
  // packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &linkaddr_node_addr);
  // PRINTF("sending %u bytes to ", packetbuf_datalen());
  // LOG_PRINT_LLADDR(packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
  // PRINTF_("\n");
  // NETSTACK_MAC.send(NULL, NULL);
  NETSTACK_LLSEC.send(NULL, NULL);
  /* If we are sending multiple packets in a row, we need to let the
     watchdog know that we are still alive. */
  watchdog_periodic();
  return 1;
}
/*--------------------------------------------------------------------*/
int sdn_net_get_last_rssi(void)
{
  return last_rssi;
}
/*--------------------------------------------------------------------*/
const struct network_driver sdn_net_driver = {
    "SDN-net",
    init,
    input,
    output};
/*--------------------------------------------------------------------*/
/** @} */