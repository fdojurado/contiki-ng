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
 *         Data structures for controller node's info
 * \author
 *         Fernando Jurado <fdo.jurado@gmail.com>
 */
#ifndef SDN_DS_NODE_H_
#define SDN_DS_NODE_H_

#include "contiki.h"
#include "net/linkaddr.h"
#include "sys/stimer.h"
#include "sdn-ctrl-types.h"

/**
 * Notifications
 */

/** \brief The default nbr_table entry (when
 * UIP_DS6_NBR_MULTI_IPV6_ADDRS is disabled), that implements nbr
 * cache */
typedef struct sdn_ds_node
{
    struct sdn_ds_node *next;
    linkaddr_t addr;        // Node's ID
    int16_t energy;         // remaing energy of node
    uint8_t rank;           // Node's rank
    uint8_t prev_ranks;     // # of previous rank nodes
    uint8_t next_ranks;     // # of following rank nodes
    uint8_t total_ranks;    // total node of other rank
    uint8_t total_nb;       // total neighbors
    uint8_t alive;          // Flag to check whether the node still alive
    struct stimer lifetime; //node lifetime
} sdn_ds_node_t;

#if SDN_DS_NODE_NOTIFICATIONS
/* Event constants for the uip-ds6 route notification interface. The
   notification interface allows for a user program to be notified via
   a callback when a NBR has been added or removed and when the
   system has added or removed a default route. */
#define SDN_DS_NODE_NOTIFICATION_ADD 0
#define SDN_DS_NODE_NOTIFICATION_RM 1
#define SDN_DS_NODE_NOTIFICATION_CH 2

typedef void (*sdn_ds_node_notification_callback)(int event,
                                                  const sdn_ds_node_t *node);
struct sdn_ds_node_notification
{
    struct sdn_ds_node_notification *next;
    sdn_ds_node_notification_callback callback;
};

void sdn_ds_node_notification_add(struct sdn_ds_node_notification *n,
                                  sdn_ds_node_notification_callback c);

void sdn_ds_node_notification_rm(struct sdn_ds_node_notification *n);
/*--------------------------------------------------*/
#endif

void sdn_ds_node_init(void);

/**
 * Add a neighbor cache for a specified IPv6 address, which is
 *  associated with a specified link-layer address
 * \param addr the address of a node to add
 * \param energy current energy of the node to add
 * \param rank the rank of the node to add
 * \param prev_ranks number of previous ranks of node to add
 * \param next_ranks number of next ranks of node to add
 * \param total_nb number of total neighbors of node to add
 * \param alive flag to represent if the node still alive
 * \return the address of a newly added nbr cache on success, NULL on
 * failure
*/
sdn_ds_node_t *sdn_ds_node_add(const linkaddr_t *addr,
                               int16_t energy,
                               uint8_t rank,
                               uint8_t prev_ranks,
                               uint8_t next_ranks,
                               uint8_t total_nb,
                               uint8_t alive);
/**
 * Remove a neighbor cache
 * \param * the address of a neighbor cache to remove
 * \return 1 on success, 0 on failure (nothing was removed)
 */
void sdn_ds_node_rm(linkaddr_t *);

/**
 * Get the neighbor cache associated with a specified IPv6 address
 * \param ipaddr an IPv6 address used as a search key
 * \return the pointer to a neighbor cache on success, NULL on failure
 */
sdn_ds_node_t *sdn_ds_node_lookup(const linkaddr_t *addr);

/**
 * Return the number of neighbor caches
 * \return the number of neighbor caches in use
 */
int sdn_ds_node_num(uint8_t *num_nodes);

/**
 * Calculates the node energy average.
 * \return the node energy average.
 */
unsigned long sdn_ds_node_energy_average();
/**
 * Calculates the total energy of the network.
 * \return the total energy of the network.
 */
unsigned long sdn_ds_node_total_energy();

/**
 * Return the maximum index of nodes
 * \return the number maximum index in table
 */
int sdn_ds_node_max_index(void);

/**
 * Get the first neighbor cache in nbr_table
 * \return the pointer to the first neighbor cache entry
 */
sdn_ds_node_t *sdn_ds_node_head(void);
/**
 * Get the first neighbor cache in nbr_table
 * \return the pointer to the first neighbor cache entry
 */
int sdn_ds_node_max_rank(void);

/**
 * Get the next neighbor cache of a specified one
 * \param nbr the pointer to a neighbor cache
 * \return the pointer to the next one on success, NULL on failure
 */
sdn_ds_node_t *sdn_ds_node_next(sdn_ds_node_t *node);

/**
 * The callback function to update link-layer stats in a neighbor
 * cache
 * \param status MAC return value defined in mac.h
 * \param numtx the number of transmissions happened for a packet
 */
//void sdn_ds_link_callback(int status, int numtx);

/**
 * The housekeeping function called periodically
 */
void sdn_ds_node_periodic(void);
#endif