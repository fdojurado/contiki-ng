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
 *         File to look in a tree structure
 * \author
 *         Fernando Jurado <fjurado@student.unimelb.edu.au>
 *
 *
 */
#ifndef SDN_DS_DFS_H_
#define SDN_DS_DFS_H_
#include "sdn-ctrl-types.h"
#include "contiki.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "sdn.h"
/*--------------------------------------------------- 
*		This struct is for finding parents and childs 
*				1
&			   /
*			  2----3
*			 /	    \
*			5--6     7
*
*---------------------------------------------------*/
typedef struct node
{
    struct node *next;    // This is to add siblings to the node
    struct node *sibling; // This is to add siblings to the node
    struct node *child;   // This is to add child to the node.
    linkaddr_t addr;      // This is the node number
} node_t;

void dfs_init(void);
node_t *new_node(const linkaddr_t *addr);
void remove_node(node_t *e);
void dfs_node_flush_all(void);
void dfs_print_nodes(void);
node_t *add_sibling(node_t *n, const linkaddr_t *data);
node_t *add_child(node_t *n, const linkaddr_t *data);
int numberChilds(node_t *n);
void find_childs(node_t *node);
int node_is_child(node_t *node, const linkaddr_t *n);
struct nodes_ids *dfs(node_t *v, const linkaddr_t *w, const uint8_t *n, struct nodes_ids *neighbor, uint8_t *discovered);
node_t *dfs_node_lookup(const linkaddr_t *addr);
#endif