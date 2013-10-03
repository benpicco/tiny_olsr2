#ifndef DUPLICATE_H_
#define DUPLICATE_H_

#include <stdbool.h>

#include "common/avl.h"
#include "common/netaddr.h"

struct olsr_node {
	struct avl_node node;

	struct netaddr* addr;		/* node addr */
	struct netaddr* last_addr;	/* node that announced this node */
	struct netaddr* next_addr;	/* neighbor addr to send packets to for this node*/
	uint16_t seq_no;			/* last seq_no from last_addr */
	time_t expires;				/* time when this tuple is invalid */
	uint8_t distance;			/* hops between us and the node */
#ifdef ENABLE_DEBUG
	char* name;					/* node name from graph.gv */
#endif
};

void add_olsr_node(struct netaddr* addr, struct netaddr* last_addr, uint16_t seq_no, uint8_t vtime, uint8_t distance, char* name);
bool is_known_msg(struct netaddr* src, uint16_t seq_no);
void olsr_init();
void print_topology_set();

#endif