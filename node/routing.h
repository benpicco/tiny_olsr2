#ifndef ROUTING_H_
#define ROUTING_H_

#include "node.h"

void add_free_node(struct olsr_node* node);
void remove_free_node(struct olsr_node* node);
void fill_routing_table(void);
bool pending_nodes_exist(void);

#endif
