/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
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
 *         Methods for SDN-DFS.c. 
 *         It builds the packet for setting up the network
 * \author
 *         Fernando Jurado <fjurado@student.unimelb.edu.au>
 *
 *
 *         
 *          pkt building & parsing
 *
 */
#include "sdn-ds-dfs.h"
#include "sdn-ds-edge.h"
#include "sdn-ctrl-types.h"

/* Log configuration */
#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

// static void print_visited(uint8_t *visited, uint8_t n);

/* This MEMB() definition defines a memory pool from which we allocate
   neighbor entries. */
LIST(dfs_nodes_table);

MEMB(dfs_nodes_memb, node_t, NODE_CACHES);
/*---------------------------------------------------------------------------*/
void dfs_init(void)
{
    list_init(dfs_nodes_table);
    memb_init(&dfs_nodes_memb);
}
/*----------------------------------------------------------
    Creates a new node
------------------------------------------------------------*/
node_t *new_node(const linkaddr_t *addr)
{
    node_t *e = NULL;

    /* Avoid inserting duplicate entries. */
    e = dfs_node_lookup(addr);
    if (e == NULL)
    {
        /* Allocate a new entry */
        e = memb_alloc(&dfs_nodes_memb);
        if (e == NULL)
        {
            PRINTF("dfs: could not allocated node!\n");
            return e;
        }
        linkaddr_copy(&e->addr, addr);
        e->sibling = NULL;
        e->child = NULL;

        /* New entry goes first. */
        list_add(dfs_nodes_table, e);

        PRINTF("dfs: adding a new node %d.%d\n", e->addr.u8[0], e->addr.u8[1]);
    }
    else
    {
        PRINTF("dfs: node already exists.\n");
    }

    return e;
}
/*---------------------------------------------------------------------------*/
node_t *dfs_node_lookup(const linkaddr_t *addr)
{
    node_t *e, *dfs_node;
    dfs_node = NULL;

    /* Find route */
    for (e = list_head(dfs_nodes_table); e != NULL; e = e->next)
    {
        if (linkaddr_cmp(&e->addr, addr))
        {
            dfs_node = e;
        }
    }
    return dfs_node;
}
/*----------------------------------------------------------
    Removes node
------------------------------------------------------------*/
void remove_node(node_t *e)
{
    if (e != NULL)
    {
        PRINTF("removing node %d.%d\n", e->addr.u8[0], e->addr.u8[1]);
        list_remove(dfs_nodes_table, e);
        memb_free(&dfs_nodes_memb, e);
    }
}
void dfs_node_flush_all(void)
{
    node_t *e;

    while (1)
    {
        e = list_pop(dfs_nodes_table);
        if (e != NULL)
        {
            memb_free(&dfs_nodes_memb, e);
        }
        else
        {
            break;
        }
    }
}
/*---------------------------------------------------------------------------
  Function to print all nodes in dfs 
---------------------------------------------------------------------------*/
void dfs_print_nodes(void)
{
    node_t *e;
    node_t *f;
    PRINTF("Printing dfs node table:\n");
    for (e = list_head(dfs_nodes_table); e != NULL; e = e->next)
    {
        PRINTF("data: %d.%d siblings ", e->addr.u8[0], e->addr.u8[1]);
        f = e;
        while (f->sibling)
        {
            f = f->sibling;
            PRINTF("%d.%d,", f->addr.u8[0], f->addr.u8[1]);
        }
        if (e->child != NULL)
        {
            PRINTF(" childs: ");
            f = e;
            f = f->child;
            PRINTF("%d.%d, ", f->addr.u8[0], f->addr.u8[1]);
            while (f->sibling)
            {
                f = f->sibling;
                PRINTF("%d.%d, ", f->addr.u8[0], f->addr.u8[1]);
            }
        }
        PRINTF("\n");
    }
}
/*----------------------------------------------------------
    This function adds a sibling.
------------------------------------------------------------*/
node_t *add_sibling(node_t *n, const linkaddr_t *addr)
{
    if (n == NULL)
        return NULL;

    while (n->sibling)
        n = n->sibling;

    return (n->sibling = new_node(addr));
}
/*----------------------------------------------------------
    This function adds a child.
------------------------------------------------------------*/
node_t *add_child(node_t *n, const linkaddr_t *addr)
{
    PRINTF(" (1) add child to node %d.%d\n",
           n->addr.u8[0], n->addr.u8[1]);
    if (n == NULL)
        return NULL;
    PRINTF(" (2) add child to node %d.%d\n",
           n->addr.u8[0], n->addr.u8[1]);
    if (n->child)
    {
        PRINTF(" (3) add sibling to child %d.%d\n",
               n->addr.u8[0], n->addr.u8[1]);
        return add_sibling(n->child, addr);
    }
    else
    {
        PRINTF(" (4) add child to node %d.%d\n",
               n->addr.u8[0], n->addr.u8[1]);
        return ((n->child) = new_node(addr));
    }
}
/*----------------------------------------------------------
    This function returns the number of childs of node n
------------------------------------------------------------*/
int numberChilds(node_t *n)
{
    if (n == NULL)
        return 0;
    node_t *na;
    na = n;
    uint8_t count = 0;
    //PRINTF("Printing childs for %d.\n", na->data);
    if (na->child == NULL)
        return 0;
    PRINTF("%d.%d, ",
           na->child->addr.u8[0],
           na->child->addr.u8[1]);
    count++;
    na = na->child;
    while (na->sibling)
    {
        na = na->sibling;
        PRINTF("%d.%d, ",
               na->addr.u8[0],
               na->addr.u8[1]);
        count++;
    }
    PRINTF("\n");
    PRINTF("node %d.%d has %d childs.\n", n->addr.u8[0], n->addr.u8[1], count);
    return count;
}
/*----------------------------------------------------------
    This function finds childs
    node : ptr to the root node
    n: number of vertex
    added_nodes: ptr to the already added nodes
------------------------------------------------------------*/
void find_childs(node_t *node)
{
    sdn_ds_edge_t *s; // used to iterate in the List.
    // uint8_t data, nbr;
    for (s = sdn_ds_edge_head(); s != NULL; s = s->next)
    {
        PRINTF("    (%d.%d, %d.%d)\n",
               s->scr.u8[0],
               s->scr.u8[1],
               s->dest.u8[0],
               s->dest.u8[1]);
        PRINTF("    root data: %d.%d\n", node->addr.u8[0], node->addr.u8[1]);
        // data = s->addr.u8[0];
        // nbr = neighbor->u8[0];
        if (linkaddr_cmp(&s->scr, &node->addr))
        // if (data == node->data)
        {
            /* check whether we already have added that node */
            if (dfs_node_lookup(&s->dest) == NULL)
            {
                PRINTF("adding child %d.%d to node %d.%d\n",
                       s->dest.u8[0], s->dest.u8[1],
                       node->addr.u8[0], node->addr.u8[1]);
                add_child(node, &s->dest);
                //memcpy(&added_nodes[neighbor->u8[0]], add_child(node, neighbor->u8[0]), sizeof(node_t));
            }
        }
        if (linkaddr_cmp(&s->dest, &node->addr))
        {
            /* check whether we already have added that node */
            if (dfs_node_lookup(&s->scr) == NULL)
            {
                PRINTF("adding child %d.%d to node %d.%d\n",
                       s->scr.u8[0], s->scr.u8[1],
                       node->addr.u8[0], node->addr.u8[1]);
                add_child(node, &s->scr);
                //memcpy(&added_nodes[s->addr.u8[0]], add_child(node, s->addr.u8[0]), sizeof(node_t));
            }
        }
        PRINTF("    (2) (%d.%d, %d.%d)\n",
               s->scr.u8[0],
               s->scr.u8[1],
               s->dest.u8[0],
               s->dest.u8[1]);
        PRINTF("    (2) root data: %d.%d\n", node->addr.u8[0], node->addr.u8[1]);
    }
}
/*----------------------------------------------------------
    This function tells whether n is a child of node
    node : ptr to the node
    n: node to test
------------------------------------------------------------*/
int node_is_child(node_t *node, const linkaddr_t *addr)
{
    node_t *na = node;
    if (na->child == NULL)
        return 0;
    na = na->child;
    if (linkaddr_cmp(&na->addr, addr))
        return 1;
    while (na->sibling)
    {
        na = na->sibling;
        if (linkaddr_cmp(&na->addr, addr))
            return 1;
    }
    return 0;
}
/*----------------------------------------------------------
    This function finds the path from vertex v to w and
    returns its neighbor to foward packets to w.
    v : ptr to the starting node
    w: node to be found
    visited : ptr to the nodes visited so far
    added_nodes : ptr to nodes that has been added
    n: number of vertex
    neighbor: neighbor to reach node w.
    disovered: ptr to discovered flag, it has to be declare
    outside of the function.
------------------------------------------------------------*/
struct nodes_ids *dfs(node_t *v, const linkaddr_t *w, const uint8_t *n, struct nodes_ids *neighbor, uint8_t *discovered)
{
    PRINTF("dfs from %d.%d to %d.%d\n",
           v->addr.u8[0], v->addr.u8[1],
           w->u8[0], w->u8[1]);
    node_t *s = NULL;
    uint8_t number_childs, i;
    struct nodes_ids *id, *id2 = NULL;
    /* label node v as discovered */
    PRINTF("Labeling node %d.%d as visited\n", v->addr.u8[0], v->addr.u8[1]);
    id = nodes_ids_lookup(&v->addr);
    if (id == NULL)
        return NULL;
    id->visited = 1;
    // *(visited + v->data) = 1;
    print_nodes_ids(n);
    /* for all edges from v to w in G.adjacentEdges(v) do */
    find_childs(v);
    PRINTF("Child of %d.%d: ", v->addr.u8[0], v->addr.u8[1]);
    number_childs = numberChilds(v);
    if (!number_childs)
        PRINTF("\nnode %d.%d does not have childs.\n", v->addr.u8[0], v->addr.u8[1]);
    dfs_print_nodes();
    for (i = 0; i < number_childs; i++)
    {
        if (!*discovered) //if it has not been discovered yet
        {
            if (i == 0) // First child encountered
            {
                s = v->child; // save child
                id2 = nodes_ids_lookup(&s->addr);
                if ((!id2->visited) && id2 != NULL) // has this child been visited before?
                {
                    PRINTF("child %d.%d not yet visited.\n", s->addr.u8[0], s->addr.u8[1]);
                    // if (s->data == *w)
                    if (linkaddr_cmp(&s->addr, w))
                    {
                        PRINTF("node found\n");
                        *discovered = 1; // w has been discovered
                        break;
                    }
                    neighbor = id2; // save neighbor to W
                    dfs(s, w, n, neighbor, discovered);
                }
            }
            if (i > 0) // then it is a sibling
            {
                s = s->sibling; //save child/sibling
                id2 = nodes_ids_lookup(&s->addr);
                if ((!id2->visited) && id2 != NULL) // has this child been visited before?
                {
                    PRINTF("sibling %d.%d not yet visited.\n", s->addr.u8[0], s->addr.u8[1]);
                    if (linkaddr_cmp(&s->addr, w))
                    {
                        PRINTF("node found\n");
                        *discovered = 1; // w has been discovered
                        break;
                    }
                    neighbor = id2;
                    dfs(s, w, n, neighbor, discovered);
                }
            }
        }
    }
    neighbor = id2; // save neighbor to W
    if (neighbor != NULL)
        PRINTF("neighbor to node %d.%d\n",
               neighbor->addr->u8[0],
               neighbor->addr->u8[1]);
    return neighbor;
}
/*----------------------------------------------------------
    prints visited nodes
------------------------------------------------------------*/
// static void print_visited(uint8_t *visited, uint8_t n)
// {
//     uint8_t i = 0;
//     for (i = 0; i < n + 1; i++) //initialize data to zero for each node
//     {
//         PRINTF("node visited (%d): %d\n", i, *(visited + i));
//     }
// }