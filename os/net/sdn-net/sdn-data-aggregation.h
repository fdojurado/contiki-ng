/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
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
 *         Configuration routes to destinations. This is done to aggregate
 *          all routes config to a node in a single packet.
 * \author
 *         Fernando Jurado <fjurado@student.unimelb.edu.au>
 *
 *
 */
#ifndef SDN_DATA_AGGREGATION_H_
#define SDN_DATA_AGGREGATION_H_

#include "sd-wsn.h"
#include "linkaddr.h"
#include "lib/list.h"

void sdn_data_aggregation_init(void);

/** \brief An entry in the config routes table */
typedef struct sdn_data_aggregation
{
    struct sdn_data_aggregation *next;
    linkaddr_t addr;
    LIST_STRUCT(seq_list); // List of sequences stored for this node.
} sdn_data_aggregation_t;

/** \brief An entry in the seq list */
typedef struct sdn_seq_list
{
    struct sdn_seq_list *next;
    uint16_t seq,
        temp,
        humidty;
} sdn_seq_list_t;

/** \name config routes basic routines */
/** @{ */
#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
sdn_data_aggregation_t *sdn_data_aggregation_lookup(const linkaddr_t *);
sdn_data_aggregation_t *sdn_data_aggregation_add(const linkaddr_t *,
                                                 const sdn_seq_list_t *);
#endif

#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
void sdn_data_aggregation_rm(sdn_data_aggregation_t *);
#endif

#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
int sdn_data_aggregation_num_packets(void);
#endif

#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
sdn_data_aggregation_t *sdn_data_aggregation_head(void);
sdn_data_aggregation_t *sdn_data_aggregation_next(sdn_data_aggregation_t *);
#endif

#if !(SDN_CONTROLLER || SERIAL_SDN_CONTROLLER)
void sdn_data_aggregation_flush_all(void);
#endif

#endif