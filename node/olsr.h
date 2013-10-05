#ifndef DUPLICATE_H_
#define DUPLICATE_H_

#include <stdbool.h>

#include "common/avl.h"
#include "common/netaddr.h"

#include "node.h"

void add_olsr_node(struct netaddr* addr, struct netaddr* last_addr, uint16_t seq_no, uint8_t vtime, uint8_t distance, char* name);
bool is_known_msg(struct netaddr* src, uint16_t seq_no);
void print_topology_set();

#endif