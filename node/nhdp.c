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
		n->last_addr = netaddr_use(local_addr);
		n->next_addr = netaddr_use(n->addr);
		n->distance = 1;
		h1_deriv(n)->link_quality = HYST_SCALING;
		h1_deriv(n)->pending = 1;

		n->node.key = n->addr;
		avl_insert(&olsr_head, &n->node);
	} else if (n->type != NODE_TYPE_NHDP) {
		DEBUG("\tconverting olsr node %s to nhdp node",
			netaddr_to_string(&nbuf[0], n->addr));
		n = _node_replace(n);
	}

	add_other_route(n, local_addr, vtime);

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
			n2 = calloc(1, sizeof(struct olsr_node));
			n2->addr = netaddr_dup(addr);
			n2->node.key = n2->addr;
			avl_insert(&olsr_head, &n2->node);
		} else {
			DEBUG("\t%s became a 2-hop neighbor", netaddr_to_string(&nbuf[0], addr));
			remove_other_route(n2, next_addr);
			netaddr_free(n2->next_addr);
			netaddr_free(n2->last_addr);
		}
		n2->distance = 2;
		n2->next_addr = netaddr_reuse(next_addr);
		n2->last_addr = netaddr_use(n2->next_addr); /* next_addr == last_addr */
		n2->expires = time_now() + vtime;
#ifdef ENABLE_DEBUG
		n2->name = name;
#endif

		n1->mpr_neigh++;

		sched_routing_update();

		return;
	}

	if (n2->distance == 1)
		return;

	if (n2->distance == 2) {
		if (netaddr_cmp(n2->next_addr, next_addr) == 0) {
			n2->expires = time_now() + vtime;	/* found node, update vtim */
			return;
		}

		struct nhdp_node* n1_old = h1_deriv(get_node(n2->next_addr));

		if (n1_old == NULL) // || n1_old->pending ?
			return;

		/* different route to 2-hop neighbor found */
		if (n1_old->mpr_neigh > n1->mpr_neigh + 1) {
			add_other_route(n2, next_addr, vtime);
			return;
		}

		DEBUG("\tswitching MPR");	// TODO: update other_routes
		if (n1_old != NULL)
			n1_old->mpr_neigh--;
		n2->next_addr = netaddr_reuse(next_addr);
		n2->last_addr = netaddr_use(n2->next_addr); /* next_addr == last_addr */
		n1->mpr_neigh++;
		sched_routing_update();
	}
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
