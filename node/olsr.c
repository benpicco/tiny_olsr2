#include <stdlib.h>
#include <time.h>

#include "common/netaddr.h"

#include "olsr.h"
#include "util.h"
#include "debug.h"
#include "routing.h"

int olsr_node_cmp(struct olsr_node* a, struct olsr_node* b) {
	return netaddr_cmp(a->addr, b->addr);
}

struct olsr_node* _new_olsr_node(struct netaddr* addr) {
	struct olsr_node* n = calloc(1, sizeof(struct olsr_node));
	n->addr = netaddr_dup(addr);
	n->node.key = n->addr;
	avl_insert(&olsr_head, &n->node);
	return n;
}

/*
 * last_addr was removed, update all children
 * this should set alternative routes for children if availiable
 */
void _update_children(struct netaddr* last_addr) {
	DEBUG("update_children(%s)", netaddr_to_string(&nbuf[0], last_addr));
	struct olsr_node *node, *safe;
	avl_for_each_element_safe(&olsr_head, node, node, safe) {
		if (node->last_addr != NULL && netaddr_cmp(node->last_addr, last_addr) == 0) {
			node->distance = 255;
			node->next_addr = NULL;
			node->last_addr = NULL;
			netaddr_free(node->last_addr);
			netaddr_free(node->next_addr);
			// remove node from free_nodes?
			// shouldn't this be recursive?
		}
	}
}

void _remove_olsr_node(struct olsr_node* node) {
	avl_remove(&olsr_head, &node->node);

	remove_free_node(node);

	if (node->distance == 2) {
		struct nhdp_node* n1 = h1_deriv(get_node(node->last_addr));
		if (n1 != NULL)
			n1->mpr_neigh--; // TODO: select new MPR
	}

	if (node->distance > 1)
		_update_children(node->addr);

	netaddr_free(node->next_addr);
	netaddr_free(node->last_addr);
	netaddr_free(node->addr);
	free(node);
}

/*
 * iterate over all elements and remove expired entries
 */
void remove_expired() {
	struct olsr_node *node, *safe;
	avl_for_each_element_safe(&olsr_head, node, node, safe) {
		if (node->expires < time(0)) {
			DEBUG("%s (%s) expired, removing it",
				node->name, netaddr_to_string(&nbuf[0], node->addr));

			_remove_olsr_node(node);
		}
	}
}

void add_olsr_node(struct netaddr* addr, struct netaddr* last_addr, uint8_t vtime, uint8_t distance, char* name) {
	struct olsr_node* n = get_node(addr);

	if (n == NULL || n->last_addr == NULL) {
		DEBUG("new olsr node: %s, last hop: %s - distance: %d", 
			netaddr_to_string(&nbuf[0], addr),
			netaddr_to_string(&nbuf[1], last_addr),
			distance);
		if (n == NULL)
			n = _new_olsr_node(addr);
		n->last_addr = netaddr_reuse(last_addr);
		n->distance = distance;
		n->expires = time(0) + vtime;
#ifdef ENABLE_DEBUG
		n->name = name;
#endif

		add_free_node(n);

		return;
	}

	/* diverging from the spec to save a little space (spec says keep all paths) */
	// TODO: change this when handling timeouts
	if (distance > n->distance) {
		DEBUG("discarding longer (%d > %d) route for %s (%s) via %s",
			distance, n->distance,
			n->name, netaddr_to_string(&nbuf[0], n->addr),
			netaddr_to_string(&nbuf[1], last_addr));
		return;
	}

	DEBUG("updating TC entry for %s (%s)", n->name, netaddr_to_string(&nbuf[0], n->addr));

	/* we found a better route */
	if (netaddr_cmp(last_addr, n->last_addr) != 0) {

		/* discard alternative route that is not an improvement */
		if (distance == n->distance)
			return;

		DEBUG("shorter route found (old: %d hops over %s new: %d hops over %s)",
			n->distance, netaddr_to_string(&nbuf[0], n->last_addr),
			distance, netaddr_to_string(&nbuf[1], last_addr));

		assert(is_valid_neighbor(n->addr, last_addr));

		netaddr_free(n->last_addr);
		n->last_addr = netaddr_reuse(last_addr);
		n->distance = distance;

		add_free_node(n);
	} else if (distance < n->distance) {
		/* we have the same last_addr, but a shorter route */
		/* obtain new next_hop */
		n->distance = distance;

		add_free_node(n);
	}

	n->expires = time(0) + vtime;
}

bool is_known_msg(struct netaddr* addr, uint16_t seq_no, uint8_t vtime) {
	struct olsr_node* node = get_node(addr);
	if (!node) {
		node = _new_olsr_node(addr);
		node->seq_no = seq_no;
		node->expires = time(0) + vtime;
		node->distance = 255;	// use real distance here?
		return false;
	}

	uint16_t tmp = node->seq_no;
	node->seq_no = seq_no;
	/*	S1 > S2 AND S1 - S2 < MAXVALUE/2 OR
		S2 > S1 AND S2 - S1 > MAXVALUE/2	*/
	if ((seq_no > tmp && seq_no - tmp < (1 << 15)) ||
		(seq_no < tmp && tmp - seq_no > (1 << 15)) )
		return false;

	return true;
}

#ifdef ENABLE_DEBUG
void print_topology_set() {
	DEBUG();
	DEBUG("---[ Topology Set ]--");

	struct olsr_node* node;
	avl_for_each_element(&olsr_head, node, node) {
		DEBUG("%s (%s) => %s; %d hops, next: %s, %zd s [%d] %s valid",
			netaddr_to_string(&nbuf[0], node->addr),
			node->name,
			netaddr_to_string(&nbuf[1], node->last_addr),
			node->distance,
			netaddr_to_string(&nbuf[2], node->next_addr),
			node->expires - time(0),
			node->seq_no,
			is_valid_neighbor(node->addr, node->last_addr) ? "" : "not"
			);
	}
	DEBUG("---------------------");
	DEBUG();
}
#else
void print_topology_set() {}
#endif