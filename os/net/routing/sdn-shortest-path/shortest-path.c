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
#include "shortest-path.h"
#include "sdn-net/sdn-controller/sdn-ds-node.h"
#include "sdn-net/sdn-controller/sdn-ds-node-route.h"
#include "sdn-net/sdn-controller/sdn-ds-edge.h"
#include "sdn-net/sdn-controller/sdn-ctrl-types.h"
#include "sdn-net/sdn.h"
#include <limits.h>
#include "stdbool.h"
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

PROCESS(shortestpath_process, "Shortest path");

static int minDistance(int dist[], bool sptSet[], uint8_t n);
/*---------------------------------------------------------------------------
  Function to compute the Shortest Path Alg.
    Return 0 if the network needs to be reconfigured.
---------------------------------------------------------------------------*/
static PT_THREAD(compute(struct pt *pt))
{
    PT_BEGIN(pt);
    PT_WAIT_UNTIL(pt, flag);
    set_computing_flag();
    PRINTF("computing Shortest Path Problem:\n");
    struct nodes_ids *id, *id2;
    sdn_ds_route_node_t *r;
    uint8_t i, j;
    int result = 0;
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
    /* Cost matrix */
    static int8_t graph[NODE_CACHES][NODE_CACHES]; // we do not want to include node 0.
    /* Initialize every cost to 0 */
    for (i = 0; i < num_vx; i++)
    {
        for (j = 0; j < num_vx; j++)
        {
            graph[i][j] = 0;
        }
    }
    /* set values to matrix */
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
        }
    }
    /* Print vector */
    PRINTF("printing vector: \n");
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
    /* Compute Dijkstra's Algorithm */
    PT_YIELD(pt);
    result = dijkstra(graph, ctrl_node_id(), num_vx);
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
int dijkstra(int8_t graph[NODE_CACHES][NODE_CACHES],
             struct nodes_ids *src,
             uint8_t n)
{
    PRINTF("computing dijkstra algorithm to source %d.%d (%d)\n",
           src->addr.u8[0], src->addr.u8[1],
           src->name);
    int updated = 1;
    struct nodes_ids *id, *id2;
    sdn_ds_edge_t *edges;
    int i, count, u, v, j, k;
    int dist[n];    // The output array.  dist[i] will hold the shortest
                    // distance from src to i
    bool sptSet[n]; // sptSet[i] will true if vertex i is included in shortest
    // path tree or shortest distance from src to i is finalized
    // Initialize all distances as INFINITE and stpSet[] as false
    int pred[NODE_CACHES];
    for (i = 0; i < n; i++)
        dist[i] = INT_MAX, sptSet[i] = false, pred[i] = src->name;
    // Distance of source vertex from itself is always 0
    dist[src->name] = 0;

    // Find shortest path for all vertices
    for (count = 1; count <= n - 1; count++)
    {
        // Pick the minimum distance vertex from the set of vertices not
        // yet processed. u is always equal to src in the first iteration.
        u = minDistance(dist, sptSet, n);
        id = nodes_ids_id_lookup(u);
        PRINTF("Minimum distance: %d.%d (%d)\n", id->addr.u8[0], id->addr.u8[1], u);

        // Mark the picked vertex as processed
        sptSet[u] = true;
        PRINTF("sptSet[%d]: %d\n", u, sptSet[u]);
        PRINTF("sptSet[%d]: %d\n", u + 1, sptSet[u + 1]);
        // Update dist value of the adjacent vertices of the picked vertex.
        for (v = 0; v < n; v++)
        {

            // Update dist[v] only if is not in sptSet, there is an edge from
            // u to v, and total weight of path from src to  v through u is
            // smaller than current value of dist[v]
            if (!sptSet[v] && graph[u][v] && dist[u] != INT_MAX && dist[u] + graph[u][v] < dist[v])
            {
                dist[v] = dist[u] + graph[u][v];
                id = nodes_ids_id_lookup(u);
                id2 = nodes_ids_id_lookup(v);
                PRINTF("edge %d.%d (%d) to %d.%d (%d) with value: %d (total %d)\n",
                       id->addr.u8[0], id->addr.u8[1], u,
                       id2->addr.u8[0], id2->addr.u8[1], v,
                       graph[u][v],
                       dist[v]);
                PRINTF("predecesor of node %d.%d (%d): %d.%d (%d)\n",
                       id2->addr.u8[0], id2->addr.u8[1], v,
                       id->addr.u8[0], id->addr.u8[1], u);
                pred[v] = u;
            }
        }
    }
    // print the constructed distance array
    k = 0;
    for (i = 0; i < n; i++)
        if (i != src->name)
        {
            id = nodes_ids_id_lookup(i);
            PRINTF("Distance of node %d (%d.%d)= %d\n", i, id->addr.u8[0], id->addr.u8[1], dist[i]);

            j = i;
            if ((k < n - 1) && (dist[i] != INT_MAX))
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
static int minDistance(int dist[], bool sptSet[], uint8_t n)
{
    // Initialize min value
    int min = INT_MAX, min_index = 0;
    int v;
    struct nodes_ids *id;
    sdn_ds_node_t *node;

    for (v = 0; v < n; v++)
    {
        id = nodes_ids_id_lookup(v);
        node = sdn_ds_node_lookup(&id->addr);
        if (sptSet[v] == false && dist[v] <= min)
            if (v == ctrl_node_id()->name || (node != NULL && node->energy > 0))
                min = dist[v], min_index = v;
    }

    return min_index;
}
/*---------------------------------------------------------------------------*/
/** Period for uip-ds6 periodic task*/
#ifndef SDN_ROUTING_CONF_PERIOD
#define SDN_ROUTING_PERIOD (CLOCK_SECOND / 10)
#else
#define SDN_ROUTING_PERIOD SDN_NC_CONF_PERIOD
#endif

PROCESS_THREAD(shortestpath_process, ev, data)
{
    PROCESS_BEGIN();

    static struct pt compute_pt;

    static struct etimer timer;

    etimer_set(&timer, SDN_ROUTING_PERIOD);

    PRINTF("shortestpath_process started.\n");

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