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
#define SDN_CONF_MAX_ROUTES 10
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

/* Set to enable TSCH security */
#ifndef WITH_SECURITY
#define WITH_SECURITY 0
#endif /* WITH_SECURITY */

/*******************************************************/
/******************* Configure TSCH ********************/
/*******************************************************/
/* Five nines reliability paper used the config below */
#define TSCH_CONF_KEEPALIVE_TIMEOUT (30 * CLOCK_SECOND)
#define TSCH_CONF_MAX_KEEPALIVE_TIMEOUT (60 * CLOCK_SECOND)
/* Disable the 6TiSCH minimal schedule */
#define TSCH_SCHEDULE_CONF_WITH_6TISCH_MINIMAL 0

/* Size of the application-specific schedule; a number relatively prime to the hopseq length */
#define APP_SLOTFRAME_SIZE 17

#if WITH_SECURITY

/* Enable security */
#define LLSEC802154_CONF_ENABLED 1

#endif /* WITH_SECURITY */

/* Logs */
#define LOG_CONF_LEVEL_RPL LOG_LEVEL_NONE     /* Controller logs */
#define LOG_CONF_LEVEL_IPV6 LOG_LEVEL_NONE     /* sdn-ds-route, sdn-ds-nbr, nb discovery, sdn-routing */
#define LOG_CONF_LEVEL_6LOWPAN LOG_LEVEL_NONE /* sd-net */
#define LOG_CONF_LEVEL_TCPIP LOG_LEVEL_NONE    /* sdn, advertisement, sd-wsn */
#define LOG_CONF_LEVEL_MAC LOG_LEVEL_DBG    /* MAC, TSCH */
/*---------------------------------------------------------------------------*/
#endif /* PROJECT_CONF_H_ */
