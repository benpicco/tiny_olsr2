#include <stdlib.h>
#include <time.h>

#include "nhdp.h"
#include "util.h"
#include "debug.h"
#include "node.h"
#include "routing.h"
#include "constants.h"

#include "common/avl.h"

struct olsr_node* _node_replace(struct olsr_node* old_n, struct olsr_node* new_n) {
	avl_remove(&olsr_head, &old_n->node);
	remove_free_node(old_n);

	new_n->addr = old_n->addr;
	new_n->seq_no = old_n->seq_no;

	netaddr_free(old_n->next_addr);
	netaddr_free(old_n->last_addr);

	free(old_n);

	return new_n;
} 

struct olsr_node* add_neighbor(struct netaddr* addr, uint8_t vtime) {
	struct olsr_node* n = get_node(addr);

	if (n == NULL || n->last_addr == NULL || n->distance > 1) {
		if (n == NULL) {
			DEBUG("\tadding new neighbor: %s", netaddr_to_string(&nbuf[0], addr));
			n = calloc(1, sizeof(struct nhdp_node));
			n->addr = netaddr_dup(addr);
		} else {
			DEBUG("\t%s became a neighbor", netaddr_to_string(&nbuf[0], addr));
			n = _node_replace(n, calloc(1, sizeof(struct nhdp_node)));
		}
		n->type = NODE_TYPE_1_HOP;
		n->last_addr = netaddr_use(local_addr);
		n->next_addr = netaddr_use(n->addr);
		n->distance = 1;

		n->node.key = n->addr;
		avl_insert(&olsr_head, &n->node);
	}

	n->expires = time(0) + vtime;

	if (h1_deriv(n)->link_quality < HYST_LOW) {
		h1_deriv(n)->pending = 1;
		// should we remove all children yet?
	}

	if (h1_deriv(n)->link_quality > HYST_HIGH) {
		h1_deriv(n)->pending = 0;
		sched_routing_update();
	}

	assert(is_valid_neighbor(n->addr, n->last_addr));

	return n;
}

void add_2_hop_neighbor(struct netaddr* addr, struct netaddr* next_addr, uint8_t vtime, char* name) {
	struct nhdp_node* n1 = h1_deriv(get_node(next_addr));

	assert (n1 != NULL);

	if (n1->pending) {
		DEBUG("%s is pending, ignoring 2-hop node %s", h1_super(n1)->name, name);
		return;
	}

	struct olsr_node* n2 = get_node(addr);

	if(n2 == NULL || n2->last_addr == NULL || n2->distance > 2) {
		if (n2 == NULL) {
			n2 = calloc(1, sizeof(struct nhdp_2_hop_node));
			n2->addr = netaddr_dup(addr);
		} else {
			DEBUG("\t%s became a 2-hop neighbor", netaddr_to_string(&nbuf[0], addr));
			n2 = _node_replace(n2, calloc(1, sizeof(struct nhdp_2_hop_node)));
		}
		n2->type = NODE_TYPE_2_HOP;
		n2->distance = 2;
		n2->next_addr = netaddr_reuse(next_addr);
		n2->last_addr = netaddr_use(n2->next_addr); /* next_addr == last_addr */
		n2->expires = time(0) + vtime;
#ifdef ENABLE_DEBUG
		n2->name = name;
#endif
		n2->node.key = n2->addr;
		avl_insert(&olsr_head, &n2->node);

		n1->mpr_neigh++;

		assert(is_valid_neighbor(n2->addr, n2->last_addr));

		sched_routing_update();

		return;
	}

	if (n2->distance == 1)
		return;

	if (n2->distance == 2) {
		n2->expires = time(0) + vtime;	/* found node, update vtim */
		struct nhdp_node* n1_old = h1_deriv(get_node(n2->next_addr));

		/* everything stays the same */
		if (n1_old != NULL && 
			(netaddr_cmp(n2->next_addr, next_addr) == 0 || n1_old->mpr_neigh > n1->mpr_neigh + 1)) {
			return;
		}

		DEBUG("\tswitching MPR");
		if (n1_old != NULL)
			n1_old->mpr_neigh--;
		n2->next_addr = netaddr_reuse(next_addr);
		n2->last_addr = netaddr_use(n2->next_addr); /* next_addr == last_addr */
		n1->mpr_neigh++;
		sched_routing_update();
	}

	assert(is_valid_neighbor(n2->addr, n2->last_addr));
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