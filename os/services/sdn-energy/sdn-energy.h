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
 *         Header file for forwarding packets in SDN
 * \author
 *         Fernando Jurado <ffjla@dtu.dk>
 *
 *
 */
#ifndef SDN_ENERGY_H
#define SDN_ENERGY_H
#include "contiki.h"

extern signed long energy;

// #ifdef RX_CONF_CURRENT
// #define RX_CURRENT RX_CONF_CURRENT
// #else /* ROUTE_CONF_ENTRIES */
// #define RX_CURRENT 21.8
// #endif /* ROUTE_CONF_ENTRIES */

// #ifdef CPU_CONF_CURRENT
// #define CPU_CURRENT CPU_CONF_CURRENT
// #else /* ROUTE_CONF_ENTRIES */
// #define CPU_CURRENT 1.8
// #endif /* ROUTE_CONF_ENTRIES */

// #ifdef LPM_CONF_CURRENT
// #define LPM_CURRENT LPM_CONF_CURRENT
// #else /* ROUTE_CONF_ENTRIES */
// #define LPM_CURRENT 0.545
// #endif /* ROUTE_CONF_ENTRIES */

#ifdef NODE_CONF_INIT_ENERGY
#define NODE_INIT_ENERGY NODE_CONF_INIT_ENERGY
#else /* ROUTE_CONF_ENTRIES */
#define NODE_INIT_ENERGY 65535L
#endif /* ROUTE_CONF_ENTRIES */

#ifdef NODE_CONF_VOLTAGE
#define NODE_VOLTAGE NODE_CONF_VOLTAGE
#else /* ROUTE_CONF_ENTRIES */
#define NODE_VOLTAGE 3L
#endif /* ROUTE_CONF_ENTRIES */

PROCESS_NAME(sdn_energy);

#endif