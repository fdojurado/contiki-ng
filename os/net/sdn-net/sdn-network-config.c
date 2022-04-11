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
 *         Header for the Contiki/SD-WSN neighbor advertisement
 * \author
 *         Fernando Jurado <fdo.jurado@gmail.com>
 */

#include "sdn-network-config.h"
#include "sd-wsn.h"
#include "sdn-ds-route.h"
#include "net/sdn-net/sdn.h"
#include "stdbool.h"
#if SDN_CONTROLLER
#include "routing/clustering/cluster-list.h"
#include "sdn-net/sdn-controller/sdn-ctrl-types.h"
#include "lib/random.h"
#include "sdn-controller/sdn-ds-config-routes.h"
#include "net/routing/routing.h"
#include "sdn-controller/sdn-ds-node.h"
#include "sdn-controller/sdn-ds-dfs.h"
#include "net/sdn-net/sdnbuf.h"
#endif
#include "net/routing/routing.h"

#if SERIAL_SDN_CONTROLLER
#include "sdn-controller-serial/sdn-serial.h"
#include "sdn-controller-serial/sdn-serial-protocol.h"
#include <string.h> // needed for memcpy
#endif              /* SERIAL_SDN_CONTROLLER */
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "SDN-NC"
#define LOG_LEVEL LOG_CONF_LEVEL_TCPIP

#if SDN_CONTROLLER
/** Period for uip-ds6 periodic task*/
#ifndef SDN_NC_CONF_PERIOD
#define SDN_NC_PERIOD (CLOCK_SECOND / 10)
#else
#define SDN_NC_PERIOD SDN_NC_CONF_PERIOD
#endif
#endif

#if SDN_CONTROLLER
#define PT_TIMER (CLOCK_SECOND / 10)
/* Keep track of computing state */
static int8_t is_computing = 0;
struct stimer nc_timer_nc; /**< NC timer, to schedule NC pkts */
struct timer pt_timer_pt;  /**< Protothread timer */

struct etimer nc_timer_periodic;

/* protothread functions */
struct pt main_pt;
struct pt send_pt;
struct pt build_pt;
struct pt output_pt;

uint16_t ack;
uint16_t ack_rcv;
// PT_THREAD(send_nc_pkt(void);

/* ACK list for delay acks */
#if SDN_CONTROLLER
struct ack_num
{
    struct ack_num *next;
    uint16_t ack;
};
LIST(ack_list);
MEMB(ack_list_memb, struct ack_num, 5);
#endif

#endif

/*---------------------------------------------------------------------------*/
// #if SDN_DS_NBR_NOTIFICATIONS
// static void
// neighbor_callback(int event, const sdn_ds_nbr_t *nbr)
// {

// }
// #endif
/*---------------------------------------------------------------------------*/
void sdn_nc_init(void)
{
#if SDN_CONTROLLER
    etimer_set(&nc_timer_periodic, SDN_NC_PERIOD);
    stimer_set(&nc_timer_nc, SDN_MIN_NC_INTERVAL); /* wait to network sets up */
    timer_set(&pt_timer_pt, PT_TIMER);             /* wait to network sets up */
    PT_INIT(&main_pt);
    PT_INIT(&send_pt);
    PT_INIT(&build_pt);
    PT_INIT(&output_pt);
#endif
    /* callback function when neighbor removed */
    // #if SDN_DS_NBR_NOTIFICATIONS
    //     static struct sdn_ds_nbr_notification n;
    //     sdn_ds_nbr_notification_add(&n,
    //                                 neighbor_callback);
    // #endif
    return;
}
/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER && SDN_WITH_TABLE_CHKSUM
static uint16_t
chksum(uint16_t sum, const uint8_t *data, uint16_t len)
{
    uint16_t t;
    const uint8_t *dataptr;
    const uint8_t *last_byte;

    dataptr = data;
    last_byte = data + len - 1;

    while (dataptr < last_byte)
    { /* At least two more bytes */
        t = (dataptr[0] << 8) + dataptr[1];
        sum += t;
        if (sum < t)
        {
            sum++; /* carry */
        }
        dataptr += 2;
    }

    if (dataptr == last_byte)
    {
        t = (dataptr[0] << 8) + 0;
        sum += t;
        if (sum < t)
        {
            sum++; /* carry */
        }
    }

    LOG_INFO("chksum, sum 0x%04x\n",
             sum);

    /* Return sum in host byte order. */
    return sum;
}
#endif
/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER && SDN_WITH_TABLE_CHKSUM
static uint16_t calculate_node_chksum(sdn_ds_config_route_t *rt)
{
    sdn_ds_config_route_list_t *list_rt;
    uint16_t sum = 0;

    typedef union
    {
        uint16_t u16[2];
        uint8_t u8[4];
    } data_t;

    data_t data;

    data.u16[0] = 0;
    data.u16[1] = 0;

    for (list_rt = list_head(rt->rt_list);
         list_rt != NULL;
         list_rt = list_item_next(list_rt))
    {
        LOG_INFO("calculate_table_chksum: %d.%d - %d.%d\n",
                 list_rt->dest.u8[0], list_rt->dest.u8[1],
                 list_rt->via.u8[0], list_rt->via.u8[1]);
        data.u16[0] = list_rt->dest.u16;
        data.u16[1] = list_rt->via.u16;
        sum = chksum(sum, &data.u8[0], 4);
        LOG_INFO("sum 0x%04x\n",
                 sum);
    }
    LOG_INFO("routing table chksum: sum 0x%04x\n",
             sum);
    data.u16[0] = sum;
    data.u16[1] = rt->recv_chksum;
    LOG_INFO("rcv chksum of node %d.%d: 0x%04x\n",
             rt->scr.u8[0], rt->scr.u8[1],
             rt->recv_chksum);
    sum = chksum(0, &data.u8[0], 4);

    LOG_INFO("sum with rcv chksum 0x%04x\n",
             sum);

    return (sum == 0) ? 0xffff : sdnip_htons(sum);
}
#endif
/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER
void sdn_nc_ack_input()
{
    ack_rcv = sdnip_htons(SDN_NC_ACK_BUF->ack);
    LOG_INFO("NC-ACK received: %u\n", ack_rcv);
}
#endif
/*---------------------------------------------------------------------------*/
#if !SDN_CONTROLLER || SERIAL_SDN_CONTROLLER
void send_ack(uint16_t ack)
{
    LOG_INFO("Sending ACK (%u) to %d.%dn",
             ack + 1,
             ctrl_addr.u8[0], ctrl_addr.u8[1]);
    /* layer 3 packet */
    SDN_IP_BUF->vap = (0x01 << 5) | SDN_PROTO_NC_ROUTE;
    /* Total length */
    sdn_len = SDN_IPH_LEN + SDN_NCH_LEN;

    SDN_IP_BUF->tlen = sdn_len;

    SDN_IP_BUF->ttl = 0x40;

    SDN_IP_BUF->scr.u16 = sdnip_htons(linkaddr_node_addr.u16);

    SDN_IP_BUF->dest.u16 = sdnip_htons(ctrl_addr.u16);

    SDN_IP_BUF->hdr_chksum = 0;

    /* Build NC routing packet packet */
    SDN_NC_ROUTE_BUF->payload_len = 0;
    SDN_NC_ROUTE_BUF->seq = 0;
    SDN_NC_ROUTE_BUF->ack = ack + 1;


    SDN_NC_ROUTE_BUF->pkt_chksum = 0;
    SDN_NC_ROUTE_BUF->pkt_chksum = ~sdn_ncchksum(SDN_NC_ROUTE_BUF->payload_len);

    print_buff(sdn_buf, sdn_len, true);
    /* For the serial controller, the nxthop should be different */
    return;
}
#endif /* !SDN_CONTROLLER || SERIAL_SDN_CONTROLLER */
/*---------------------------------------------------------------------------*/
#if !SDN_CONTROLLER || SERIAL_SDN_CONTROLLER
void sdn_nc_input(void)
{
    // linkaddr_t via, dest, ch_addr;
    uint8_t i;
    uint16_t ack;
    ack = sdnip_htons(SDN_NC_ROUTE_BUF->seq);
    /* Calculate number of routes */
    uint8_t num_rt = SDN_NC_ROUTE_BUF->payload_len / 3;
    // if (ch_addr.u16 != 0xFFFF)
    // {
    //     cluster_head = 0;
    //     LOG_INFO("NC processing %d routes (CH %d.%d)\n", num_rt, ch_addr.u8[0], ch_addr.u8[1]);
    // }
    // else
    // {
    //     cluster_head = 1;
    //     LOG_INFO("NC processing %d routes\n", num_rt);
    // }
    for (i = 0; i < num_rt; i++)
    {
        // via.u16 = sdnip_htons(SDN_NC_ROUTE_PAYLOAD(i)->via.u16);
        // dest.u16 = sdnip_htons(SDN_NC_ROUTE_PAYLOAD(i)->dest.u16);
        // sdn_ds_route_add(&dest, 0, &via, CONTROLLER);
    }

    /* Send ACK */
    send_ack(ack);
    return;
}
#endif /* !SDN_CONTROLLER || SERIAL_SDN_CONTROLLER */
/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER
static void ack_list_flush(void)
{
    struct ack_num *ack;

    ack = list_head(ack_list);

    while (ack != NULL)
    {
        list_remove(ack_list, ack);
        memb_free(&ack_list_memb, ack);
        ack = list_head(ack_list);
    }
    return;
}
#endif
/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER
static struct ack_num *ack_lookup(uint16_t *ack)
{
    if (ack == NULL)
        return NULL;

    struct ack_num *element, *found;

    found = NULL;

    for (element = list_head(ack_list);
         element != NULL;
         element = list_item_next(element))
    {
        if (*ack == element->ack)
            found = element;
    }
    if (found != NULL)
    {
        LOG_INFO("Ack found: %u\n",
                 found->ack);
    }
    return found;
}
#endif
/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER
static int acknowledgment(uint16_t *ack)
{
    /* Check whether this ack is in the ack list or not */
    int result = 0;
    uint16_t ack_rcv = *ack;

    struct ack_num *ack_ptr;

    ack_rcv--;

    ack_ptr = ack_lookup(&ack_rcv);

    if (ack_ptr != NULL)
        result = 1;

    return result;
}
#endif
/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER
static struct ack_num *ack_add(uint16_t *ack)
{
    if (ack == NULL)
        return NULL;

    struct ack_num *ack_ptr;

    ack_ptr = ack_lookup(ack);

    if (ack_ptr != NULL)
    {
        LOG_INFO("ack already exists.\n");
        return NULL;
    }

    if (list_length(ack_list) >= 5)
    {
        /* Deallocate one ack. */
        ack_ptr = list_tail(ack_list);
        list_remove(ack_list, ack_ptr);
        memb_free(&ack_list_memb, ack_ptr);
    }

    ack_ptr = memb_alloc(&ack_list_memb);

    if (ack_ptr == NULL)
    {
        LOG_INFO("Couldn't allocate more acks.\n");
        return NULL;
    }

    ack_ptr->ack = *ack;
    list_add(ack_list, ack_ptr);
    LOG_INFO("adding ack: %u\n",
             ack_ptr->ack);

    return ack_ptr;
}
#endif
/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER
static PT_THREAD(send_nc_output(linkaddr_t *addr))
{
    PT_BEGIN(&output_pt);
    static sdn_ds_config_route_t *rt;
    rt = sdn_ds_config_routes_lookup(addr);
    /* Only send NC packets if the tables doesnt match */
#if SDN_WITH_TABLE_CHKSUM
    if (calculate_node_chksum(rt) != 0xffff)
    {
#endif
        static const linkaddr_t *nxthop;
#if ROUTING_SDN_AGG
        cluster_list_t *cluster;
#endif
        static struct timer timeout;
        static uint8_t rtx;
        nxthop = NETSTACK_ROUTING.nexthop(addr);
        if (nxthop != NULL)
        {
            rtx = 0;
            ack_list_flush();
#if SDN_WITH_TABLE_CHKSUM
            /* Delete any routes that are not current */
            sdn_ds_config_routes_delete_current(addr);
#endif /* SDN_WITH_TABLE_CHKSUM */
            do
            {
                rtx++;
                /* payload size */
                int8_t payload_size = SDN_NC_LEN * sdn_ds_config_routes_num_routes_node(addr);
                /* layer 3 packet */
                SDN_IP_BUF->vahl = (0x01 << 5) | SDN_IPH_LEN;
                /* Total length */
                sdn_len = SDN_IPH_LEN + SDN_CPH_LEN + payload_size;

                SDN_IP_BUF->len = sdn_len;

                SDN_IP_BUF->ttl = 0x40;

                SDN_IP_BUF->proto = SDN_PROTO_CP;

                SDN_IP_BUF->scr.u16 = sdnip_htons(linkaddr_node_addr.u16);

                SDN_IP_BUF->dest.u16 = sdnip_htons(addr->u16);

                SDN_IP_BUF->ipchksum = 0;
                SDN_IP_BUF->ipchksum = ~sdn_ipchksum();

                /* Control packet */
                SDN_CP_BUF->type = SDN_PROTO_NC;

                SDN_CP_BUF->len = payload_size;

                /* Use rank as ACK number */
                ack = random_rand();
                ack_rcv = 0;
                while (ack == ack_rcv)
                {
                    ack_rcv = random_rand();
                }
                SDN_CP_BUF->rank = sdnip_htons(ack);

                LOG_INFO("Sending NC to %d.%d via %d.%d with ACK %u (%d)\n",
                         addr->u8[0], addr->u8[1],
                         nxthop->u8[0], nxthop->u8[1],
                         ack,
                         rtx);

                /* Add ack to ack list */
                ack_add(&ack);

#if ROUTING_SDN_AGG
                cluster = cluster_list_lookup(addr);

                if (cluster != NULL && !cluster->CH)
                    SDN_CP_BUF->energy = sdnip_htons(cluster->ch_addr.u16);
                else
                    SDN_CP_BUF->energy = 0xFFFF;
#else
            SDN_CP_BUF->energy = 0;
#endif /* ROUTING_SDN_AGG */

                /* Put routes's info in payload */
                uint8_t count = 0;
                sdn_ds_config_route_list_t *list_rt;

                if (rt == NULL)
                {
                    LOG_INFO("No NC routes for %d.%d\n", addr->u8[0], addr->u8[1]);
                    break;
                }

                for (list_rt = list_head(rt->rt_list);
                     list_rt != NULL;
                     list_rt = list_item_next(list_rt))
                {
                    SDN_NC_BUF(count)->via.u16 = sdnip_htons(list_rt->via.u16);
                    SDN_NC_BUF(count)->dest.u16 = sdnip_htons(list_rt->dest.u16);
                    count++;
                }

                SDN_CP_BUF->cpchksum = 0;
                SDN_CP_BUF->cpchksum = ~sdn_cpchksum(SDN_CP_BUF->len);

                /* Update statistics */
                SDN_STAT(++sdn_stat.ip.sent);
                SDN_STAT(++sdn_stat.cp.nc);
                SDN_STAT(sdn_stat.cp.nc_bytes += sdn_len);

                print_buff(sdn_buf, sdn_len, true);

                sdnbuf_set_attr(SDNBUF_ATTR_MAX_MAC_TRANSMISSIONS, 3);

                sdn_ip_output(nxthop);

                /* Set timeout */
                timer_set(&timeout, CLOCK_SECOND * 7);

                PT_WAIT_UNTIL(&output_pt, acknowledgment(&ack_rcv) || timer_expired(&timeout));
            } while (timer_expired(&timeout) && rtx < 8);

            /* Now, delete packets from the queue. */
            // sdn_ds_config_routes_rm(rt);
            rt->acked = 1;
        }
#if SDN_WITH_TABLE_CHKSUM
    }
    else
    {
        rt->acked = 1;
    }
#endif
    PT_END(&output_pt);
}
#endif
/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER
static PT_THREAD(send_nc_pkt(void))
{
    PT_BEGIN(&send_pt);
    /* Timer for timeout */
    static linkaddr_t *addr;
    /* Clear acked flag */
    sdn_ds_config_routes_clear_flags();
    do
    {
        /* check whether we have somenthing queued */
        // PT_WAIT_UNTIL(&send_pt, sdn_ds_config_routes_head() != NULL); /* Wait until the queue is not empty */
        addr = sdn_config_routes_min_depth();
        if (addr != NULL)
        {
            LOG_INFO("New config packet to send (%d.%d)\n",
                     addr->u8[0], addr->u8[1]);
            PT_SPAWN(&send_pt, &output_pt, send_nc_output(addr));
        }
    } while (addr != NULL);
    PT_END(&send_pt);
}
#endif
/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER
static void clean_nodes(const uint8_t *n, uint8_t *discovered)
{
    // uint8_t i = 0;
    // uint8_t *ptr = NULL;
    /* For every node, we do... */
    dfs_node_flush_all(); // remove every node in added nodes
    // for (ptr = buff; ptr < buff + n; ptr++) //initialize data to zero for each node
    // {
    //     *ptr = 0;
    // }
    clean_visited(n);
    *discovered = 0; // reset discovered flag
}
#endif
/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER
static PT_THREAD(sdn_nc_build(void))
{
    PT_BEGIN(&build_pt);
    is_computing = 1;
    struct nodes_ids *id, *neighbor_to_ctr, *neighbor_to_node, *neighbor_node;
    static struct nodes_ids *controller_id;
    controller_id = ctrl_node_id();
    static uint8_t num_vx;
    sdn_ds_node_num(&num_vx); // number of vertices
    // num_vx = sdn_ds_node_num();
    // uint8_t j;
    // LOG_INFO("NODE_CACHES=%d size of visited_buff=%d (%d,%d)\n", NODE_BUFF,
    //        sizeof(visited_buff), NELEMS(visited_buff), sizeof(visited_buff[0]));
    // uint8_t *visited = visited_buff; //this allows to reuse the sdn buffer
    uint8_t discovered = 0; // flag used for discovering nodes
    // uint8_t neighbor_to_ctr;  // Neighbor to the SDN controller
    // uint8_t neighbor_node; // Neighbor to the SDN node
    // uint8_t neighbor_to_node; // Neighbor to the SDN node
    // uint8_t nearest_nb;              // Nearest neighbor to node
    // static uint8_t max_index = 0;
    // sdn_pkt_t pkt;
    // linkaddr_t addr, addr_nb, dest;
    static sdn_ds_node_t *node;
    // uint8_t *buff_ptr = sdn_config_buf;
    node_t *scr;
    /* rename nodes */
    if (!rename_nodes(&num_vx))
    {
        LOG_INFO("num of vx greater than buffer.\n");
        return 0;
    }
#if SDN_WITH_TABLE_CHKSUM
    sdn_ds_config_routes_clear_current();
#endif /* SDN_WITH_TABLE_CHKSUM */
    /* Here, we want to find the path from nodes to controller */
    for (node = sdn_ds_node_head();
         node != NULL;
         node = sdn_ds_node_next(node))
    {
        if (node->energy > 0)
        {
            // max_index = sdn_ds_node_max_index();
            /* map adress */
            id = nodes_ids_lookup(&node->addr);
            LOG_INFO("node to test: %d.%d rank %d (name=%d)\n",
                     id->addr.u8[0],
                     id->addr.u8[1],
                     node->rank,
                     id->name);
            neighbor_to_ctr = id;
            do
            {
                id = neighbor_to_ctr;
                LOG_INFO("j=%d (%d.%d) neighbor_to_ctr=%d (%d.%d)\n",
                         id->name,
                         id->addr.u8[0], id->addr.u8[1],
                         neighbor_to_ctr->name,
                         neighbor_to_ctr->addr.u8[0], neighbor_to_ctr->addr.u8[1]);
                // LOG_INFO("no. elems in array=%d\n", NELEMS(visited_buff));
                clean_nodes(&num_vx, &discovered);
                scr = new_node(&id->addr);
                LOG_INFO("source data: %d.%d (j=%d) (%d.%d)\n",
                         scr->addr.u8[0],
                         scr->addr.u8[1],
                         id->name,
                         id->addr.u8[0], id->addr.u8[1]);
                neighbor_to_ctr = dfs(scr, &controller_id->addr, &num_vx, neighbor_to_ctr, &discovered);
                LOG_INFO("neighbor node to ctr of node %d (%d.%d) is %d (%d.%d)\n",
                         id->name,
                         id->addr.u8[0], id->addr.u8[1],
                         neighbor_to_ctr->name,
                         neighbor_to_ctr->addr.u8[0], neighbor_to_ctr->addr.u8[1]);
                // addr.u8[0] = j;
                // addr.u8[1] = 0;
                // addr_nb.u8[0] = neighbor_to_ctr;
                // addr_nb.u8[1] = 0;
                // }
                sdn_ds_config_routes_add(&id->addr, &controller_id->addr, &neighbor_to_ctr->addr);
            } while (neighbor_to_ctr->name != controller_id->name);
        }
        // } while (neighbor_to_ctr != ctrl_addr.u8[0]);
        PT_YIELD(&build_pt);
    }
    /* Here, we want to find the path from controller to nodes */
    static int8_t depth;
    for (node = sdn_ds_node_head();
         node != NULL;
         node = sdn_ds_node_next(node))
    {
        if (node->energy > 0)
        {
            // max_index = sdn_ds_node_max_index();
            // j = node->node_addr.u8[0];
            /* map adress */
            id = nodes_ids_lookup(&node->addr);
            depth = 0;
            neighbor_to_node = controller_id;
            LOG_INFO("Finding path %d (%d.%d) to %d (%d.%d)\n",
                     neighbor_to_node->name,
                     neighbor_to_node->addr.u8[0], neighbor_to_node->addr.u8[1],
                     id->name,
                     id->addr.u8[0], id->addr.u8[1]);
            do
            {
                neighbor_node = neighbor_to_node;
                // clean_nodes(NELEMS(visited_buff), visited_buff, &discovered);
                clean_nodes(&num_vx, &discovered);
                scr = new_node(&neighbor_node->addr);

                neighbor_to_node = dfs(scr, &id->addr, &num_vx, neighbor_to_node, &discovered);
                LOG_INFO("dest %d (%d.%d) via %d (%d.%d)\n",
                         id->name,
                         id->addr.u8[0], id->addr.u8[1],
                         neighbor_to_node->name,
                         neighbor_to_node->addr.u8[0], neighbor_to_node->addr.u8[1]);

                if (neighbor_node->name == controller_id->name)
                    sdn_ds_route_add(&id->addr, 0, &neighbor_to_node->addr, CONTROLLER);
                else
                    sdn_ds_config_routes_add(&neighbor_node->addr, &id->addr, &neighbor_to_node->addr);
                depth++;
            } while (neighbor_to_node->name != id->name);
            sdn_ds_ntwk_depth_set(&id->addr, depth);
        }
        PT_YIELD(&build_pt);
    }
    is_computing = 0;
    PT_END(&build_pt);
}
#endif
/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER
static void sdn_nc_send(void)
{
    NETSTACK_ROUTING.compute_routing_algorithm();
}
#endif
/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER
int8_t nc_computing(void)
{
    return is_computing;
}
#endif
/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER
void set_computing_flag(void)
{
    is_computing = 1;
}
#endif
/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER
void clear_computing_flag(void)
{
    is_computing = 1;
}
#endif
/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER
static void sdn_send_nc_periodic(void)
{
    // send_nc_output();
    sdn_nc_send();
}
#endif
/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER
static PT_THREAD(pt_timer_callback(void))
{
    PT_BEGIN(&main_pt);

    while (1)
    {
        PT_WAIT_UNTIL(&main_pt, NETSTACK_ROUTING.network_modified());
        NETSTACK_ROUTING.clear_network_modified();
#if !SDN_WITH_TABLE_CHKSUM
        sdn_config_routes_flush_all();
#endif
        PT_SPAWN(&main_pt, &build_pt, sdn_nc_build());
        PT_SPAWN(&main_pt, &send_pt, send_nc_pkt());
    }
    PT_END(&main_pt);
}
#endif
/*---------------------------------------------------------------------------*/
#if SDN_CONTROLLER
void sdn_nc_periodic(void)
{
    /* Periodic ND sending */
    // LOG_INFO("periodic.\n");
    if (stimer_expired(&nc_timer_nc) /* && (sdn_len == 0) */)
    {
        sdn_send_nc_periodic();
        LOG_INFO("send nc periodic\n");
        stimer_reset(&nc_timer_nc);
    }
    if (timer_expired(&pt_timer_pt) /* && (sdn_len == 0) */)
    {
        pt_timer_callback();
        timer_reset(&pt_timer_pt);
    }
    etimer_reset(&nc_timer_periodic);
}
#endif