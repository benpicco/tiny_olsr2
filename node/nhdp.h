#ifndef NHDP_H_
#define NHDP_H_

#include "common/avl.h"
#include "common/netaddr.h"

#include "node.h"

bool send_tc_messages;		/* only send TC messages when we are selected as MPR */

void nhdp_init(void);

struct olsr_node* add_neighbor(struct netaddr* addr, uint8_t vtime);

/*
 * add a new neighbor of n
 * may fail if n is not known
 */
void add_2_hop_neighbor(struct netaddr* addr, struct netaddr* next_addr, uint8_t vtime, char* name);

void print_neighbors(void);

#endif /* NHDP_H_ */
