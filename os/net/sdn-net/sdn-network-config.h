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
 *         Header for the Contiki/SD-WSN interface
 * \author
 *         Fernando Jurado <fdo.jurado@gmail.com>
 */

#ifndef SDN_NETWORK_CONFIG_H
#define SDN_NETWORK_CONFIG_H

#include "sys/stimer.h"
// #include "net/linkaddr.h"

#if SDN_CONTROLLER
#ifndef SDN_CONF_MIN_NC_INTERVAL
#define SDN_MIN_NC_INTERVAL 60 * 12 // At least every 10 mins
#else
#define SDN_MIN_NC_INTERVAL SDN_CONF_MIN_NC_INTERVAL
#endif
#endif

#if SDN_CONTROLLER
extern struct etimer nc_timer_periodic; /**< Timer for periodic NA */
#endif

/** \brief Initialize NC structures */
void sdn_nc_init(void);

#if !SDN_CONTROLLER
/** \brief process input packets */
void sdn_nc_input(void);
#endif

/** \brief Periodic processing of data structures */
#if SDN_CONTROLLER
void sdn_nc_periodic(void);
void sdn_nc_ack_input(void);
int8_t nc_computing(void);
void set_computing_flag(void);
void clear_computing_flag(void);
#endif

#endif