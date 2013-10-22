#include <stdlib.h>
#include <assert.h>

#include "nhdp.h"
#include "util.h"
#include "debug.h"
#include "node.h"
#include "routing.h"
#include "constants.h"

#include "common/avl.h"

struct olsr_node* _node_replace(struct olsr_node* old_n) {
	struct olsr_node* new_n = calloc(1, sizeof (struct nhdp_node));

	/* remove things that held a pointer to this */
	avl_remove(&olsr_head, &old_n->node);
	remove_free_node(old_n);

	memcpy(new_n, old_n, sizeof(struct olsr_node));
	memset(&new_n->node, 0, sizeof(new_n->node));	// just to be sure

	new_n->type = NODE_TYPE_NHDP;
	new_n->node.key = new_n->addr;
	avl_insert(&olsr_head, &new_n->node);

	free(old_n);

	return new_n;
} 

struct olsr_node* add_neighbor(struct netaddr* addr, uint8_t vtime) {
	struct olsr_node* n = get_node(addr);

	if (n == NULL) {
		DEBUG("\tadding new neighbor: %s", netaddr_to_string(&nbuf[0], addr));
		n = calloc(1, sizeof(struct nhdp_node));
		n->addr = netaddr_dup(addr);

		n->type = n->type = NODE_TYPE_NHDP;
		n->distance = 1;
		h1_deriv(n)->link_quality = HYST_SCALING;
		h1_deriv(n)->pending = 1;

		n->node.key = n->addr;
		avl_insert(&olsr_head, &n->node);

		add_free_node(n);
	} else if (n->type != NODE_TYPE_NHDP) {
		DEBUG("\tconverting olsr node %s to nhdp node",
			netaddr_to_string(&nbuf[0], n->addr));
		n = _node_replace(n);

		add_free_node(n);
	}

	add_other_route(n, local_addr, vtime);

	return n;
}

#ifdef ENABLE_DEBUG
void print_neighbors(void) {
	struct olsr_node* node;

	DEBUG("1-hop neighbors:");
	avl_for_each_element(&olsr_head, node, node) {
		if (node->distance == 1)
			DEBUG("\tneighbor: %s (%s) (mpr for %d nodes)",
				node->name,
				netaddr_to_string(&nbuf[0], node->addr),
				h1_deriv(node)->mpr_neigh);
	}

	DEBUG("2-hop neighbors:");
	avl_for_each_element(&olsr_head, node, node) {
		if (node->distance == 2)
			DEBUG("\t%s (%s) -> %s -> %s (%s)",
				node->name, netaddr_to_string(&nbuf[0], node->addr),
				netaddr_to_string(&nbuf[1], node->next_addr),
				local_name,
				netaddr_to_string(&nbuf[2], local_addr));
	}
}
#else
void print_neighbors(void) {}
#endif
