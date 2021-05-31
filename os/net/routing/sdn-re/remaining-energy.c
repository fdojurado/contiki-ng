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
 *         it computes the Shortes path based on the Dijkstra's algorithm
 * \author
 *         Fernando Jurado <fjurado@student.unimelb.edu.au>
 *
 *
 */
#include "remaining-energy.h"
#include "sdn-net/sdn-controller/sdn-ds-node.h"
#include "sdn-net/sdn-controller/sdn-ds-node-route.h"
#include "sdn-net/sdn-controller/sdn-ds-edge.h"
#include "sdn-net/sdn.h"
#include <limits.h>
#include "stdbool.h"
#include "linkaddr.h"
#include "sdn-net/sdn-controller/sdn-ctrl-types.h"
#include "sdn-net/sdn-network-config.h"
/* Log configuration */
#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#ifdef SDN_DS_CONF_MAX_NODE_CACHES
#define NODE_CACHES SDN_DS_CONF_MAX_NODE_CACHES + 1
#else
#define NODE_CACHES 10
#endif
uint8_t table_modified;

static int flag = 0;

PROCESS(remaining_energy_process, "Remaining energy");

static int maxEnergy(int dist[],
                     bool sptSet[],
                     signed long energy[],
                     uint8_t n,
                     uint8_t rank);
static int dijkstra(int8_t graph[NODE_CACHES][NODE_CACHES],
                    int16_t energy_cost[NODE_CACHES][NODE_CACHES],
                    struct nodes_ids *,
                    uint8_t n);
/*---------------------------------------------------------------------------
  Function to compute the maximum energy path.
    Return 0 if the network needs to be reconfigured.
---------------------------------------------------------------------------*/
static PT_THREAD(compute(struct pt *pt))
{
    PT_BEGIN(pt);
    PT_WAIT_UNTIL(pt, flag);
    set_computing_flag();
    PRINTF("computing maximum remaining energy path:\n");
    struct nodes_ids *id, *id2;
    sdn_ds_route_node_t *r;
    sdn_ds_node_t *node;
    uint8_t i, j;
    static int result = 0;
    static uint8_t num_vx; //number of vertices
#if DEBUG
    uint8_t num_edge;  //number of edges
    uint8_t max_index; //Greatest node number index
#endif
    sdn_ds_node_num(&num_vx);
#if DEBUG
    num_edge = sdn_ds_node_route_num_routes();
    max_index = sdn_ds_node_max_index();
    PRINTF("max index of vx %d # vertices %d edges %d\n",
           max_index, num_vx, num_edge);
#endif
    /* rename nodes */
    if (!rename_nodes(&num_vx))
    {
        PRINTF("num of vx greater than buffer.\n");
        return 0;
    }
    /* RSSI cost matrix */
    static int8_t graph[NODE_CACHES][NODE_CACHES]; // we do not want to include node 0.
    /* Energy cost matrix */
    static int16_t energy[NODE_CACHES][NODE_CACHES]; // we do not want to include node 0.
    /* Initialize every cost to 0 */
    for (i = 0; i < num_vx; i++)
    {
        for (j = 0; j < num_vx; j++)
            graph[i][j] = energy[i][j] = 0;
    }
    /* set values to RSSI matrix */
    for (r = sdn_ds_node_route_head();
         r != NULL;
         r = sdn_ds_node_route_next(r))
    {
        /* set to the renamed node */
        id = nodes_ids_lookup(&r->scr);
        id2 = nodes_ids_lookup(&r->dest);
        if (id != NULL && id2 != NULL)
        {
            graph[id->name][id2->name] =
                graph[id2->name][id->name] =
                    (-1) * r->rssi;
            node = sdn_ds_node_lookup(&r->scr);
            if (node != NULL)
                energy[id->name][id2->name] = node->energy;
            node = sdn_ds_node_lookup(&r->dest);
            if (node != NULL)
                energy[id2->name][id->name] = node->energy;
        }
    }
    /* Print vector */
    PRINTF("printing RSSI cost matrix: \n");
    for (i = 0; i < num_vx; i++)
    {
        for (j = 0; j < num_vx; j++)
        {
            id = nodes_ids_id_lookup(i);
            id2 = nodes_ids_id_lookup(j);
            PRINTF("%d.%d - %d.%d = %d ",
                   id->addr.u8[0], id->addr.u8[1],
                   id2->addr.u8[0], id2->addr.u8[1],
                   graph[i][j]);
        }
        PRINTF("\n");
    }
    PRINTF("printing energy cost matrix: \n");
    for (i = 0; i < num_vx; i++)
    {
        for (j = 0; j < num_vx; j++)
        {
            if (i == 0)
                energy[i][j] = energy[j][i];
            id = nodes_ids_id_lookup(i);
            id2 = nodes_ids_id_lookup(j);
            PRINTF("%d.%d - %d.%d = %u ",
                   id->addr.u8[0], id->addr.u8[1],
                   id2->addr.u8[0], id2->addr.u8[1],
                   energy[i][j]);
        }
        PRINTF("\n");
    }
    /* Compute Dijkstra's Algorithm */
    PT_YIELD(pt);
    result = dijkstra(graph, energy, ctrl_node_id(), num_vx);
    PT_YIELD(pt);
    if (!result)
    {
        PRINTF("edges table modified:\n");
        table_modified = 1;
    }
    /* Delete any edges not present in sdn-ds-node-route table */
    sdn_ds_edge_t *edge;
    edge = sdn_ds_edge_head();
    while (edge != NULL)
    {
        if (sdn_ds_node_route_lookup(&edge->scr, &edge->dest) == NULL)
        {
            /* delete edge */
            PRINTF("Deleting edge %d.%d - %d.%d\n",
                   edge->scr.u8[0], edge->scr.u8[1],
                   edge->dest.u8[0], edge->dest.u8[1]);
            sdn_ds_edge_rm(edge);
            edge = sdn_ds_edge_head();
        }
        else
        {
            edge = sdn_ds_edge_next(edge);
        }
    }
    /* Max number of edges is N-1 */
    while (sdn_ds_edge_num_edges() > num_vx - 1)
    {
        PRINTF("We need to delete extra edges\n");
        edge = sdn_ds_edge_tail();
        sdn_ds_edge_rm(edge);
    }
    flag = 0;
    PT_END(pt);
}
/*---------------------------------------------------------------------------
  Function to Dijkstra's Algorithm.
  For a graph represented using adjacency matrix representation
  Returns 0 if there has been a change in the edge table
---------------------------------------------------------------------------*/
static int dijkstra(int8_t graph[NODE_CACHES][NODE_CACHES],
                    int16_t energy_cost[NODE_CACHES][NODE_CACHES],
                    struct nodes_ids *src,
                    uint8_t n)
{
    PRINTF("computing remaining energy algorithm:\n");
    int updated = 1;
    struct nodes_ids *id, *id2, *id3;
    uint8_t u_rank, v_rank;
    sdn_ds_edge_t *edges;
    sdn_ds_node_t *node, *s;
    int i, rank, u, v, j, k;
    int dist[n]; // The output array.  dist[i] will hold the shortest
    // distance from src to i
    signed long energy[n]; // max energy array
    bool sptSet[n];          // sptSet[i] will true if vertex i is included in shortest
    // Initialize all distances as INFINITE and stpSet[] as false
    int pred[NODE_CACHES];
    for (i = 0; i < n; i++)
        sptSet[i] = false, pred[i] = -1, energy[i] = 0;

    sptSet[0] = true;
    energy[src->name] = INT_MAX;

    for (rank = 0; rank <= sdn_ds_node_max_rank(); rank++)
    {
        for (s = sdn_ds_node_head();
             s != NULL;
             s = sdn_ds_node_next(s))
        {
            if (s->rank == rank && s->energy > 0)
            {
                id3 = nodes_ids_lookup(&s->addr);
                while (pred[id3->name] == -1)
                {
                    PRINTF("node: %d.%d rank: %d\n",
                           s->addr.u8[0], s->addr.u8[1],
                           s->rank);
                    // Pick the minimum distance vertex from the set of vertices not
                    // yet processed. u is always equal to src in the first iteration.
                    for (i = 0; i < n; i++)
                    {
                        id = nodes_ids_id_lookup(i);
                        PRINTF("energy of node %d.%d: %ld (flag=%d)\n",
                               id->addr.u8[0], id->addr.u8[1],
                               energy[i], sptSet[i]);
                    }
                    u = maxEnergy(dist, sptSet, energy, n, rank - 1);
                    id = nodes_ids_id_lookup(u);
                    node = sdn_ds_node_lookup(&id->addr);
                    if (!linkaddr_cmp(&id->addr, &ctrl_node_id()->addr))
                        u_rank = node->rank;
                    else
                        u_rank = 0;
                    PRINTF("Node with most energy: %d.%d\n",
                           id->addr.u8[0], id->addr.u8[1]);

                    // Mark the picked vertex as processed
                    sptSet[u] = true;
                    PRINTF("sptSet of node %d.%d: %d\n",
                           id->addr.u8[0], id->addr.u8[1],
                           sptSet[u]);
                    // Update dist value of the adjacent vertices of the picked vertex.
                    for (v = 0; v < n; v++)
                    {
                        id = nodes_ids_id_lookup(v);
                        if (!linkaddr_cmp(&id->addr, &ctrl_addr))
                            node = sdn_ds_node_lookup(&id->addr),
                            v_rank = node->rank;
                        else
                            v_rank = 0;

                        // Update dist[v] only if is not in sptSet, there is an edge from
                        // u to v, and total weight of path from src to  v through u is
                        // smaller than current value of dist[v]
                        if (!sptSet[v] && energy_cost[u][v] && energy[u] != 0 && (u_rank == v_rank - 1) &&
                            energy[u] + energy_cost[v][u] >= energy[v])
                        {
                            // dist[v] = dist[u] + graph[u][v];
                            energy[v] = energy[u] + energy_cost[v][u];
                            id = nodes_ids_id_lookup(u);
                            id2 = nodes_ids_id_lookup(v);
                            PRINTF("edge %d.%d - %d.%d with RSSI %d energy cost %d\n",
                                   id->addr.u8[0], id->addr.u8[1],
                                   id2->addr.u8[0], id2->addr.u8[1],
                                   graph[u][v],
                                   energy_cost[v][u]);
                            PRINTF("energy of node %d.%d: %ld\n",
                                   id2->addr.u8[0], id2->addr.u8[1],
                                   energy[v]);
                            PRINTF("predecesor of node %d.%d: %d.%d\n",
                                   id2->addr.u8[0], id2->addr.u8[1],
                                   id->addr.u8[0], id->addr.u8[1]);
                            pred[v] = u;
                            PRINTF("predecesor %d\n", pred[v]);
                        }
                    }
                }
            }
        }
    }
    // }
    // print the constructed distance array
    k = 0;
    for (i = 0; i < n; i++)
        if (src->name != i && energy[i] > 0)
        {
            id = nodes_ids_id_lookup(i);
            PRINTF("Print energy of node %d.%d = %ld\n",
                   id->addr.u8[0], id->addr.u8[1],
                   energy[i]);

            j = i;
            if ((k < n - 1) && (energy[i] != 0))
            {
                id = nodes_ids_id_lookup(i);
                id2 = nodes_ids_id_lookup(pred[j]);
                edges = sdn_ds_edge_lookup(&id->addr, &id2->addr);
                if (edges == NULL)
                    sdn_ds_edge_add(&id->addr, &id2->addr), updated = 0;
                k++;
            }
            id = nodes_ids_id_lookup(i);
            PRINTF("Path=%d.%d",
                   id->addr.u8[0], id->addr.u8[1]);
            do
            {
                j = pred[j];
                id2 = nodes_ids_id_lookup(j);
                PRINTF(" <- %d.%d",
                       id2->addr.u8[0], id2->addr.u8[1]);

            } while (j != src->name);
            PRINTF("\n");
        }
    return updated;
}
/*---------------------------------------------------------------------------
  A utility function to find the vertex with minimum distance value, from
  the set of vertices not yet included in shortest path tree
---------------------------------------------------------------------------*/
static int maxEnergy(int dist[], bool sptSet[], signed long energy[], uint8_t n, uint8_t rank)
{
    // Initialize min value
    int min_index = 0;
    uint8_t node_rank;
    signed long max_energy = 0;
    // linkaddr_t addr;
    sdn_ds_node_t *node;
    struct nodes_ids *id;
    int v;

    for (v = 0; v < n; v++)
        if (sptSet[v] == false &&
            energy[v] >= max_energy)
        {
            id = nodes_ids_id_lookup(v);
            if (id != NULL)
            {
                if (!linkaddr_cmp(&id->addr, &ctrl_addr))
                {
                    node = sdn_ds_node_lookup(&id->addr);
                    node_rank = node->rank;
                }
                else
                {
                    node_rank = 0;
                }
                if (node_rank == rank || node_rank == 0)
                {
                    max_energy = energy[v];
                    min_index = v;
                }
            }
        }
    // PRINTF("node: %d\n", min_index);
    PRINTF("maximum energy: %ld\n", max_energy);
    return min_index;
}
/*---------------------------------------------------------------------------*/
/** Period for uip-ds6 periodic task*/
#ifndef SDN_ROUTING_CONF_PERIOD
#define SDN_ROUTING_PERIOD (CLOCK_SECOND / 10)
#else
#define SDN_ROUTING_PERIOD SDN_NC_CONF_PERIOD
#endif

PROCESS_THREAD(remaining_energy_process, ev, data)
{
    PROCESS_BEGIN();

    static struct pt compute_pt;

    static struct etimer timer;

    etimer_set(&timer, SDN_ROUTING_PERIOD);

    PRINTF("remaining_energy_process started.\n");

    while (1)
    {
        PROCESS_YIELD();
        if (ev == PROCESS_EVENT_POLL)
        {
            PRINTF("event polled.\n");
            flag = 1;
            table_modified = 0;
        }
        if (ev == PROCESS_EVENT_TIMER && data == &timer)
        {
            compute(&compute_pt);
            etimer_reset(&timer);
        }
    }

    PROCESS_END();
}