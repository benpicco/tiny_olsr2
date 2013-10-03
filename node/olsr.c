#include <stdlib.h>
#include <time.h>
#ifdef DEBUG
#include <stdio.h>
#endif

#include "common/netaddr.h"
#include "common/avl_comp.h"

#include "olsr.h"

static struct avl_tree olsr_head;


struct olsr_node* get_olsr_node(struct netaddr* addr) {
	struct olsr_node *n; // for typeof
	return avl_find_element(&olsr_head, addr, n, node);
}

void add_olsr_node(struct netaddr* addr, struct netaddr* last_addr, uint16_t seq_no, uint8_t vtime, uint8_t distance, char* name) {
	struct olsr_node* n = get_olsr_node(addr);

	if (!n) {
		n = calloc(1, sizeof(struct olsr_node));
		n->addr = netaddr_dup(addr);
		n->last_addr = netaddr_use(last_addr);
		n->expires = time(0) + vtime;
		n->seq_no = seq_no;
#ifdef DEBUG
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
		netaddr_free(n->last_addr);
		n->last_addr = netaddr_use(last_addr);
	}

	n->seq_no = seq_no;		/* is_known_msg() should have been called before */
	n->distance  = distance;
	n->expires = time(0) + vtime;
}

bool is_known_msg(struct netaddr* addr, uint16_t seq_no) {
	struct olsr_node* node;
	avl_for_each_element(&olsr_head, node, node) {
		if (netaddr_cmp(addr, node->last_addr) == 0) {
			/* handle overflow gracefully */
			if (seq_no > node->seq_no || (seq_no < 100 && node->seq_no > 65400))
				return false;
			else
				return true;
		}
	}
	return false;
}

void olsr_init() {
	avl_init(&olsr_head, avl_comp_netaddr, false);
}

#ifdef DEBUG
void print_topology_set() {
	struct netaddr_str nbuf[2];

	struct olsr_node* node;
	avl_for_each_element(&olsr_head, node, node) {
		printf("%s (%s) => %s; %d hops, %zd s [%d]\n",
			netaddr_to_string(&nbuf[0], node->addr),
			node->name,
			netaddr_to_string(&nbuf[1], node->last_addr),
			node->distance,
			node->expires - time(0),
			node->seq_no
			);
	}
}
#endif