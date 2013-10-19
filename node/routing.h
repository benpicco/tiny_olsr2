#ifndef ROUTING_H_
#define ROUTING_H_

#include "node.h"

/*
 * add a node to the list of pending nodes
 */
void add_free_node(struct olsr_node* node);

/*
 * remove a node from the list of pending nodes
 */
void remove_free_node(struct olsr_node* node);

/*
 * try to find a route for pending nodes
 */
void fill_routing_table(void);

/*
 * Schedule a routing update
 */
void sched_routing_update(void);

#endif /* ROUTING_H_ */
