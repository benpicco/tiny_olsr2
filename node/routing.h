#ifndef ROUTING_H_
#define ROUTING_H_

#include "node.h"

struct free_node;

void add_free_node(struct free_node** head, struct olsr_node* node);
void fill_routing_table (struct free_node** head);

#endif
