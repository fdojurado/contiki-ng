/*
 * Copyright (c) 2016, George Oikonomou - http://www.spd.gr
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*---------------------------------------------------------------------------*/
#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_
/*---------------------------------------------------------------------------*/
/* Energy tracking */
#define ENERGEST_CONF_ON 1
/* Neighbbor discovery period */
#define SDN_CONF_MAX_ND_INTERVAL 60 * 3
/* Neighbbor advertisement period */
#define SDN_CONF_MAX_NA_INTERVAL 60 * 4
/* Network configuration period */
#define SDN_CONF_MIN_NC_INTERVAL 60 * 14
/* Initial emergy */
#define NODE_CONF_INIT_ENERGY 20000L
/* Linkaddr size */
#define LINKADDR_CONF_SIZE 2
/* Num of max routing routes */
#define SDN_CONF_MAX_ROUTES 15
/* Flocklab deployment? */
#define FLOCKLAB_DEPLOYMENT 0
/* SDN STATISTICS? */
#define SDN_STATISTICS 1
/* Max number of neighbors in cache */
#define SDN_DS_NBR_CONF_MAX_NEIGHBOR_CACHES 10
#undef NETSTACK_CONF_NETWORK
#define NETSTACK_CONF_NETWORK sdn_net_driver
#undef NETSTACK_CONF_ROUTING
#define NETSTACK_CONF_ROUTING sdn_routing_driver
// #define QUEUEBUF_CONF_NUM 16
// #define UIP_CONF_BUFFER_SIZE 0
// #define TSCH_QUEUE_CONF_NUM_PER_NEIGHBOR 16
/* Delete unneccesary conf */
// #define SICSLOWPAN_CONF_FRAG 0
// #define UIP_CONF_UDP 0
// #define UIP_CONF_UDP_CONNS 0
// #define UIP_CONF_ND6_AUTOFILL_NBR_CACHE 0
// #define SICSLOWPAN_CONF_COMPRESSION 0
/* Logs */
#define LOG_CONF_LEVEL_RPL LOG_LEVEL_NONE     /* Controller logs */
#define LOG_CONF_LEVEL_IPV6 LOG_LEVEL_DBG     /* sdn-ds-route, sdn-ds-nbr, nb discovery, sdn-routing */
#define LOG_CONF_LEVEL_6LOWPAN LOG_LEVEL_NONE /* sd-net */
#define LOG_CONF_LEVEL_TCPIP LOG_LEVEL_DBG    /* sdn, advertisement, sd-wsn */
#define LOG_CONF_LEVEL_MAC LOG_LEVEL_NONE     /* MAC, TSCH */
/*---------------------------------------------------------------------------*/
#endif /* PROJECT_CONF_H_ */
