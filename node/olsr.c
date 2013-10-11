#include <stdlib.h>
#include <time.h>

#include "common/netaddr.h"

#include "olsr.h"
#include "util.h"
#include "debug.h"
#include "routing.h"

struct free_node* free_nodes_head = 0;

void _remove_olsr_node(struct olsr_node* node);

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
 * nodes with last_addr can now be routed in n hops 
 */
void _routing_changed(struct netaddr* last_addr, struct netaddr* next_addr, int hops) {
	DEBUG("_routing_changed(%s => %s, %d)", 
		netaddr_to_string(&nbuf[0], last_addr),
		netaddr_to_string(&nbuf[1], next_addr), hops);

	struct olsr_node* node;
	avl_for_each_element(&olsr_head, node, node) {
		if (node->last_addr != NULL && netaddr_cmp(node->last_addr, last_addr) == 0) {

			struct netaddr* tmp = node->next_addr;
			node->next_addr = netaddr_use(next_addr);
			node->distance = hops;
			netaddr_free(tmp);
			_routing_changed(node->addr, next_addr, hops + 1);
		} 
	}
}

/*
 * we don't store alternative routes (yet) so remove all child nodes if a parent dies
 */
void _remove_children(struct netaddr* last_addr) {
	struct olsr_node *node, *safe;
	avl_for_each_element_safe(&olsr_head, node, node, safe) {
		if (node->last_addr != NULL && netaddr_cmp(node->last_addr, last_addr) == 0) {
			DEBUG("\talso removing %s (%s)",
				node->name, netaddr_to_string(&nbuf[0], node->addr));
			_remove_olsr_node(node);
		}
	}
}

void _remove_expired() {
	struct olsr_node *node, *safe;
	avl_for_each_element_safe(&olsr_head, node, node, safe) {
		if (node->expires < time(0)) {
			DEBUG("%s (%s) expired, removing it",
				node->name, netaddr_to_string(&nbuf[0], node->addr));
			_remove_olsr_node(node);
		}
	}
}

void _remove_olsr_node(struct olsr_node* node) {
	avl_remove(&olsr_head, &node->node);

	if (node->distance == 2) {
		struct olsr_node* n1 = get_node(node->last_addr);
		if (n1 != NULL)
			h1_deriv(n1)->mpr_neigh--; // TODO: select new MPR
	}
	
	if (node->distance > 1)
		_remove_children(node->addr);

	netaddr_free(node->next_addr);
	netaddr_free(node->last_addr);
	netaddr_free(node->addr);
	free(node);
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

		struct netaddr* next_addr = get_node(last_addr)->next_addr;
		if (next_addr != NULL)
			n->next_addr = netaddr_use(next_addr);
		else
			add_free_node(&free_nodes_head, n);

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
		if (distance == n->distance)
			return;
		DEBUG("shorter route found (old: %d hops over %s new: %d hops over %s)",
			n->distance, netaddr_to_string(&nbuf[0], n->last_addr),
			distance, netaddr_to_string(&nbuf[1], last_addr));

		assert(is_valid_neighbor(n->addr, last_addr));

		netaddr_free(n->last_addr);
		n->last_addr = netaddr_reuse(last_addr);

		/* see if we can route the new shorter route yet */
		struct netaddr* next_addr = get_node(last_addr)->next_addr;
		if (next_addr != NULL) {
			netaddr_use(next_addr);
			netaddr_free(n->next_addr);
			n->next_addr = next_addr;

			_routing_changed(addr, next_addr, distance + 1);
		} else
			add_free_node(&free_nodes_head, n);
	}

	n->expires = time(0) + vtime;
}

bool is_known_msg(struct netaddr* addr, uint16_t seq_no, uint8_t vtime) {
	struct olsr_node* node = get_node(addr);
	if (!node) {
		node = _new_olsr_node(addr);
		node->seq_no = seq_no;
		node->expires = time(0) + vtime;
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

void olsr_update() {
	DEBUG("update routing table (%s pending nodes)", free_nodes_head ? "some" : "no");
	fill_routing_table(&free_nodes_head);
	_remove_expired();
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