#ifndef NHDP_H_
#define NHDP_H_

#include "common/avl.h"
#include "common/netaddr.h"

#include "node.h"

struct netaddr local_addr;

#ifdef ENABLE_DEBUG
char* node_name;
#endif

bool send_tc_messages;		/* only send TC messages when we are selected as MPR */

enum {
	ADD_2_HOP_OK,
	ADD_2_HOP_IS_LOCAL,
	ADD_2HOP_IS_NEIGHBOR
};

void nhdp_init();

struct olsr_node* add_neighbor(struct netaddr* addr, uint8_t linkstatus, uint8_t vtime);

/**
* add a new neighbor of n
* may fail if n is not known
*/
int add_2_hop_neighbor(struct netaddr* addr, struct netaddr* next_addr, uint8_t linkstatus, uint8_t vtime, char* name);

void print_neighbors(void);

#endif /* NHDP_H_ */
