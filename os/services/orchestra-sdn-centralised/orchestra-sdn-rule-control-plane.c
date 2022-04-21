/*
 * Copyright (c) 2015, Swedish Institute of Computer Science.
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
 */
/**
 * \file
 *         Control plane rule: a slotframe with multiple control shared link, common to all nodes
 *         in the network, used mainly for control traffic e.g., Neighbour Discovery (ND), Schedule
 *          Advertisement (SA), and Routing Advertisement (RA).
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 *         Fernando Jurado <ffjla@dtu.dk>
 */

#include "contiki.h"
#include "orchestra.h"
#include "lib/random.h"
#include "net/sdn-net/sd-wsn.h"

static uint16_t slotframe_handle = 0;

#if ORCHESTRA_EBSF_PERIOD > 0
/* There is a slotframe for EBs, use this slotframe for non-EB traffic only */
#define ORCHESTRA_COMMON_SHARED_TYPE LINK_TYPE_NORMAL
#else
/* There is no slotframe for EBs, use this slotframe both EB and non-EB traffic */
#define ORCHESTRA_COMMON_SHARED_TYPE LINK_TYPE_ADVERTISING
#endif

/* Time and channel offsets used for the control plane. This is now fixed.
Default channel size is 31. We use 4 timeslot for operation of the control plane */
// struct comm_links
// {
//   uint8_t timeoffset,
//       channeloffset;
// };

// static const struct comm_links control_link[] = {
//     {3, 3},
//     {9, 2},
//     {21, 2},
//     {15, 1},
//     {28, 0},
// };

static uint8_t rx_timeoffset = 0;
static uint8_t rx_channeloffset = 0;
static uint8_t tx_timeoffset = 0;
static uint8_t tx_channeloffset = 0;
static struct tsch_slotframe *sf_control;
/*---------------------------------------------------------------------------*/
static uint16_t
get_node_timeslot(const uint8_t rank)
{
#if ORCHESTRA_CONTROL_PERIOD > 0
  return rank % ORCHESTRA_CONTROL_PERIOD;
#else
  return 0xffff;
#endif
}
/*---------------------------------------------------------------------------*/
static uint16_t
get_node_channel_offset(const linkaddr_t *addr)
{
  if (addr != NULL && ORCHESTRA_UNICAST_MAX_CHANNEL_OFFSET >= ORCHESTRA_UNICAST_MIN_CHANNEL_OFFSET)
  {
    return ORCHESTRA_LINKADDR_HASH(addr) % (ORCHESTRA_UNICAST_MAX_CHANNEL_OFFSET - ORCHESTRA_UNICAST_MIN_CHANNEL_OFFSET + 1) + ORCHESTRA_UNICAST_MIN_CHANNEL_OFFSET;
  }
  else
  {
    return 0xffff;
  }
}
/*---------------------------------------------------------------------------*/
static int
select_packet(uint16_t *slotframe, uint16_t *timeslot, uint16_t *channel_offset)
{
  /* We select any random time and channel offset from the control link */
  if (packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE) == FRAME802154_DATAFRAME &&
      ((SDN_IP_BUF->vap == ((0x01 << 5) | SDN_PROTO_SA)) ||
       (SDN_IP_BUF->vap == ((0x01 << 5) | SDN_PROTO_RA))))
  {
    if (slotframe != NULL)
    {
      *slotframe = slotframe_handle;
    }
    if (timeslot != NULL)
    {
      *timeslot = tx_timeoffset;
    }
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
init(uint16_t sf_handle)
{
  slotframe_handle = sf_handle;
  /* Default slotframe: for broadcast or unicast to neighbors we
   * do not have a link to */
  sf_control = tsch_schedule_add_slotframe(slotframe_handle, ORCHESTRA_CONTROL_PERIOD);
  tx_timeoffset = 1;
  rx_channeloffset = 3;
  tx_channeloffset = 3;
  tsch_schedule_add_link(sf_control,
                         LINK_OPTION_RX | LINK_OPTION_TX | LINK_OPTION_SHARED,
                         ORCHESTRA_COMMON_SHARED_TYPE, &tsch_broadcast_address,
                         rx_timeoffset, rx_channeloffset, 1);
  tsch_schedule_add_link(sf_control,
                         LINK_OPTION_TX | LINK_OPTION_SHARED,
                         ORCHESTRA_COMMON_SHARED_TYPE, &tsch_broadcast_address,
                         tx_timeoffset, tx_channeloffset, 1);
}
/*---------------------------------------------------------------------------*/
static void rank_updated(const linkaddr_t *addr, uint8_t rank)
{
  /* Update the listening timeslot, which is based on the hop position */
  if (tsch_schedule_remove_link_by_timeslot(sf_control, rx_timeoffset, rx_channeloffset) &&
      (tsch_schedule_remove_link_by_timeslot(sf_control, tx_timeoffset, tx_channeloffset)))
  {
    rx_timeoffset = get_node_timeslot(rank);
    rx_channeloffset = get_node_channel_offset(addr);
    tsch_schedule_add_link(sf_control,
                           LINK_OPTION_RX | LINK_OPTION_TX | LINK_OPTION_SHARED,
                           ORCHESTRA_COMMON_SHARED_TYPE, &tsch_broadcast_address,
                           rx_timeoffset, rx_channeloffset, 1);
    tx_timeoffset = get_node_timeslot(rank + 1);
    tx_channeloffset = get_node_channel_offset(&linkaddr_node_addr);
    tsch_schedule_add_link(sf_control,
                           LINK_OPTION_SHARED | LINK_OPTION_TX,
                           LINK_TYPE_NORMAL, &tsch_broadcast_address,
                           tx_timeoffset, tx_channeloffset, 1);
  }
}
/*---------------------------------------------------------------------------*/
struct orchestra_rule control_plane = {
    init,
    NULL,
    select_packet,
    NULL,
    NULL,
    NULL,
    rank_updated,
    "control plane",
    ORCHESTRA_COMMON_SHARED_PERIOD,
};
