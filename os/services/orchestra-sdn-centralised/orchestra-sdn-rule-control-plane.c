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
struct comm_links
{
  uint8_t timeoffset,
      channeloffset;
};

static const struct comm_links control_link[] = {
    {3, 3},
    {9, 2},
    {21, 2},
    {15, 1},
    {28, 0},
};

/*---------------------------------------------------------------------------*/
static uint8_t
get_random()
{
  return random_rand() % (uint16_t)(5);
}
/*---------------------------------------------------------------------------*/
static int
select_packet(uint16_t *slotframe, uint16_t *timeslot, uint16_t *channel_offset)
{
  /* We select any random time and channel offset from the control link */
  if (slotframe != NULL)
  {
    *slotframe = slotframe_handle;
  }
  // Get random selection of timeslot and channel offset
  uint8_t num = get_random();
  if (timeslot != NULL)
  {
    *timeslot = control_link[num].timeoffset;
  }
  /* set per-packet channel offset */
  if (channel_offset != NULL)
  {
    *channel_offset = control_link[num].channeloffset;
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static void
init(uint16_t sf_handle)
{
  slotframe_handle = sf_handle;
  /* Default slotframe: for broadcast or unicast to neighbors we
   * do not have a link to */
  struct tsch_slotframe *sf_common = tsch_schedule_add_slotframe(slotframe_handle, ORCHESTRA_COMMON_SHARED_PERIOD);
  // Let's add a link for each control_link
  int num_links = sizeof control_link / sizeof control_link[0];
  int i = 0;
  while (i < num_links)
  {
    tsch_schedule_add_link(sf_common,
                           LINK_OPTION_RX | LINK_OPTION_TX | LINK_OPTION_SHARED,
                           ORCHESTRA_COMMON_SHARED_TYPE, &tsch_broadcast_address,
                           control_link[i].timeoffset, control_link[i].channeloffset, 1);
    i++;
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
    "control plane",
    ORCHESTRA_COMMON_SHARED_PERIOD,
};
