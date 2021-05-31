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
 *         Data structures for routes of node nodes
 * \author
 *         Fernando Jurado <fdo.jurado@gmail.com>
 */
#ifndef CLUSTER_LIST_H_
#define CLUSTER_LIST_H_

#include "linkaddr.h"
#include "lib/list.h"

void cluster_list_init(void);

/** \brief An entry in the routing table */
typedef struct cluster_list
{
    struct cluster_list *next;
    linkaddr_t addr, ch_addr;
    uint8_t CH;
} cluster_list_t;

/** \name Node Table basic routines */
/** @{ */
cluster_list_t *cluster_list_lookup(const linkaddr_t *addr);
cluster_list_t *cluster_list_add(const linkaddr_t *addr, const linkaddr_t *ch_addr, uint8_t CH);

void cluster_list_rm(cluster_list_t *);
void cluster_list_flush();

int cluster_list_num_clusters(void);

cluster_list_t *cluster_list_head(void);
cluster_list_t *cluster_list_tail(void);
cluster_list_t *cluster_list_next(cluster_list_t *);

#endif