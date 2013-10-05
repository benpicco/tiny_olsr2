#include <stdlib.h>
#include <time.h>

#include "common/netaddr.h"

#include "olsr.h"
#include "util.h"
#include "debug.h"

void add_olsr_node(struct netaddr* addr, struct netaddr* last_addr, uint16_t seq_no, uint8_t vtime, uint8_t distance, char* name) {
	struct olsr_node* n = get_node(addr);

	if (!n) {
		DEBUG("New olsr node: %s, last hop: %s - distance: %d", 
			netaddr_to_string(&nbuf[0], addr),
			netaddr_to_string(&nbuf[1], last_addr),
			distance);
		n = calloc(1, sizeof(struct olsr_node));
		n->addr = netaddr_dup(addr);
		n->last_addr = netaddr_reuse(last_addr);
		n->expires = time(0) + vtime;
		n->distance = distance;
		n->seq_no = seq_no;
#ifdef ENABLE_DEBUG
		n->name = name;
#endif
		n->node.key = n->addr;
		avl_insert(&olsr_head, &n->node);

		return;
	}

	/* diverging from the spec to save a little space */
	// TODO: change this when handling timeouts
	if (n->distance < distance)
		return;

	/* we found a shorter route */
	if (netaddr_cmp(last_addr, n->last_addr) != 0) {
		DEBUG("shorter route found");
		n->last_addr = netaddr_reuse(last_addr);
	}

	DEBUG("updating topology base");

	n->seq_no = seq_no;		/* is_known_msg() should have been called before */
	n->distance  = distance;
	n->expires = time(0) + vtime;
}

bool is_known_msg(struct netaddr* addr, uint16_t seq_no) {
	struct olsr_node* node;
	avl_for_each_element(&olsr_head, node, node) {
		if (node->last_addr && netaddr_cmp(addr, node->last_addr) == 0) {
			/* handle overflow gracefully */
			if (seq_no > node->seq_no || (seq_no < 100 && node->seq_no > 65400))
				return false;
			else
				return true;
		}
	}
	return false;
}

#ifdef ENABLE_DEBUG
void print_topology_set() {
	DEBUG("---[ Topology Set ]--");

	struct olsr_node* node;
	avl_for_each_element(&olsr_head, node, node) {
		DEBUG("%s (%s) => %s; %d hops, next: %s, %zd s [%d]",
			netaddr_to_string(&nbuf[0], node->addr),
			node->name,
			netaddr_to_string(&nbuf[1], node->last_addr),
			node->distance,
			netaddr_to_string(&nbuf[2], node->next_addr),
			node->expires - time(0),
			node->seq_no
			);
	}
	DEBUG("---------------------");
}
#else
void print_topology_set() {}
#endif