#include <stdlib.h>
#include <time.h>

#include "nhdp.h"
#include "util.h"
#include "debug.h"
#include "node.h"

#include "common/avl.h"

struct olsr_node* add_neighbor(struct netaddr* addr, uint8_t linkstatus, uint8_t vtime) {
	struct olsr_node* n = get_node(addr);

	if (!n) {
		DEBUG("\tadding new neighbor: %s", netaddr_to_string(&nbuf[0], addr));
		n = calloc(1, sizeof(struct nhdp_node));
		n->type = NODE_TYPE_1_HOP;
		n->addr = netaddr_dup(addr);
		n->expires = time(0) + vtime;
		n->distance = 1;
		h1_deriv(n)->linkstatus = linkstatus;

		n->node.key = n->addr;
		avl_insert(&olsr_head, &n->node);
	}

	if (n->distance > 1) {
		DEBUG("TODO: %d hop node became neighbor", n->distance);
		// TODO
	} else {
		DEBUG("\textending validity of %s (%s)", n->name, netaddr_to_string(&nbuf[0], addr));
		n->expires = time(0) + vtime;
	}

	return n;
}

int add_2_hop_neighbor(struct netaddr* addr, struct netaddr* next_addr, uint8_t linkstatus, uint8_t vtime, char* name) {
	struct nhdp_node* n1 = h1_deriv(get_node(next_addr));
	struct olsr_node* n2 = get_node(addr);

	if(!n2) {
		n2 = calloc(1, sizeof(struct nhdp_2_hop_node));
		n2->type = NODE_TYPE_2_HOP;
		n2->addr = netaddr_dup(addr);
		n2->distance = 2;
		n2->next_addr = netaddr_reuse(next_addr);
		n2->last_addr = netaddr_use(n2->next_addr); /* next_addr == last_addr */
		n2->expires = time(0) + vtime;
		h2_deriv(n2)->linkstatus = linkstatus;
#ifdef ENABLE_DEBUG
		n2->name = name;
#endif
		n2->node.key = n2->addr;
		avl_insert(&olsr_head, &n2->node);

		n1->mpr_neigh++;
		return ADD_2_HOP_OK;
	}

	if (n2->distance == 1)
		return -ADD_2HOP_IS_NEIGHBOR;

	if (n2->distance == 2) {
		n2->expires = time(0) + vtime;	/* found node, update vtim */
		struct nhdp_node* n1_old = h1_deriv(get_node(n2->next_addr));
		/* everything stays the same */
		if (netaddr_cmp(n2->next_addr, next_addr) == 0 || 
			n1_old->mpr_neigh > n1->mpr_neigh + 1) {
			return -ADD_2_HOP_OK;
		}

		DEBUG("\tswitching MPR");
		n1_old->mpr_neigh--;
		n2->next_addr = netaddr_reuse(next_addr);
		n2->last_addr = netaddr_use(n2->next_addr); /* next_addr == last_addr */
		n1->mpr_neigh++;
	} else
		DEBUG("TODO: %d hop node became 2-hop-neighbor", n2->distance);

	return ADD_2_HOP_OK;
}

#ifdef ENABLE_DEBUG
void print_neighbors(void) {
	struct olsr_node* node;

	avl_for_each_element(&olsr_head, node, node) {
		if (node->distance == 1)
			DEBUG("\tneighbor: %s (%s) (mpr for %d nodes)",
				node->name,
				netaddr_to_string(&nbuf[0], node->addr),
				h1_deriv(node)->mpr_neigh);
	}
#if 0
	avl_for_each_element(&olsr_head, node, node) {
		if (node->distance == 2)
			DEBUG("\t%s (%s) -> %s -> %s (%s)",
				node->name, netaddr_to_string(&nbuf[0], node->addr),
				netaddr_to_string(&nbuf[1], node->next_addr),
				node_name,
				netaddr_to_string(&nbuf[2], &local_addr));
	}
#endif
}
#else
void print_neighbors(void) {}
#endif