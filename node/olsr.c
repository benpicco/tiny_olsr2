#include <stdlib.h>

#include "common/netaddr.h"

#include "olsr.h"
#include "util.h"
#include "debug.h"
#include "routing.h"
#include "constants.h"
#include "list.h"

struct olsr_node* _new_olsr_node(struct netaddr* addr) {
	struct olsr_node* n = calloc(1, sizeof(struct olsr_node));
	n->addr = netaddr_dup(addr);
	n->node.key = n->addr;
	avl_insert(&olsr_head, &n->node);
	return n;
}

/*
 * find a new route for nodes that use last_addr as their default route
 * if lost_node_addr is not null, all reference to it will be removed (aka lost node)
 */
void _update_children(struct netaddr* last_addr, struct netaddr* lost_node_addr) {
	DEBUG("update_children(%s, %s)", netaddr_to_string(&nbuf[0], last_addr),
		netaddr_to_string(&nbuf[1], lost_node_addr));

	struct olsr_node *node;
	avl_for_each_element(&olsr_head, node, node) {

		if (lost_node_addr != NULL)
			remove_other_route(node, lost_node_addr);

		if (node->last_addr != NULL && netaddr_cmp(node->last_addr, last_addr) == 0) {

			if (lost_node_addr != NULL) {
				node->last_addr = netaddr_free(node->last_addr);
				node->next_addr = netaddr_free(node->next_addr);
			} else
				push_default_route(node);

			add_free_node(node);

			_update_children(node->addr, lost_node_addr);
		}
	}
}

void _olsr_node_expired(struct olsr_node* node) {
	DEBUG("_olsr_node_expired");
	node->last_addr = netaddr_free(node->last_addr);
	node->next_addr = netaddr_free(node->next_addr);
	_update_children(node->addr, NULL);

	add_free_node(node);

	sched_routing_update();

	// 1-hop neighbors will become normal olsr_nodes here, should we care?
}

void _remove_olsr_node(struct olsr_node* node) {
	DEBUG("_remove_olsr_node");
	avl_remove(&olsr_head, &node->node);

	remove_free_node(node);

	if (node->distance == 2) {
		struct nhdp_node* n1 = h1_deriv(get_node(node->last_addr));
		if (n1 != NULL)
			n1->mpr_neigh--;
	}

	_update_children(node->addr, node->addr);

	/* remove other routes from node that is about to be deleted */
	char skipped;
	struct alt_route *route, *prev;
	simple_list_for_each_safe(node->other_routes, route, prev, skipped) {
		netaddr_free(route->last_addr);
		simple_list_for_each_remove(&node->other_routes, route, prev);
	}

	netaddr_free(node->next_addr);
	netaddr_free(node->last_addr);
	netaddr_free(node->addr);
	free(node);
}

void _update_link_quality(struct nhdp_node* node) {
	if (time_now() > h1_super(node)->expires)
		node->link_quality = node->link_quality * (1 - HYST_SCALING);
	else
		node->link_quality = node->link_quality * (1 - HYST_SCALING) + HYST_SCALING;

	if (node->link_quality < HYST_LOW) {
		node->pending = 1;
		_update_children(h1_super(node)->addr, NULL);
	}

	if (node->link_quality > HYST_HIGH) {
		node->pending = 0;
		// possibly reroute children
		sched_routing_update();
	}
}

/*
 * iterate over all elements and remove expired entries
 */
void remove_expired(void) {
	struct olsr_node *node, *safe;
	avl_for_each_element_safe(&olsr_head, node, node, safe) {
		/* only use HELLO for link quality calculation */
		if (node->distance == 1)
			_update_link_quality(h1_deriv(node));

		char skipped;
		struct alt_route *route, *prev;
		simple_list_for_each_safe(node->other_routes, route, prev, skipped) {
			if (time_now() - route->expires > HOLD_TIME) {
				DEBUG("alternative route to %s (%s) via %s expired, removing it",
					node->name, netaddr_to_string(&nbuf[0], node->addr),
					netaddr_to_string(&nbuf[1], route->last_addr));
				simple_list_for_each_remove(&node->other_routes, route, prev);
			}
		}

		if (time_now() - node->expires > HOLD_TIME) {
			DEBUG("%s (%s) expired",
				node->name, netaddr_to_string(&nbuf[0], node->addr));

			if (node->other_routes == NULL)
				_remove_olsr_node(node);
			else
				_olsr_node_expired(node);
		}
	}

	fill_routing_table();
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

		if (n->distance == 0 || n->distance > distance)
			n->distance = distance;

		n->expires = time_now() + vtime;

#ifdef ENABLE_DEBUG
		n->name = name;
#endif

		add_other_route(n, last_addr, vtime);
		add_free_node(n);

		return;
	}

	if (distance > n->distance) {
		DEBUG("found longer (%d > %d) route for %s (%s) via %s",
			distance, n->distance,
			n->name, netaddr_to_string(&nbuf[0], n->addr),
			netaddr_to_string(&nbuf[1], last_addr));

		add_other_route(n, last_addr, vtime);
		return;
	}

	DEBUG("updating TC entry for %s (%s)", n->name, netaddr_to_string(&nbuf[0], n->addr));

	/* we found a better route */
	if (netaddr_cmp(last_addr, n->last_addr) != 0) {

		if (distance == n->distance) {
			add_other_route(n, last_addr, vtime);
			return;
		}

		DEBUG("shorter route found (old: %d hops over %s new: %d hops over %s)",
			n->distance, netaddr_to_string(&nbuf[0], n->last_addr),
			distance, netaddr_to_string(&nbuf[1], last_addr));

		n->distance = distance;

		push_default_route(n);
		add_other_route(n, last_addr, vtime);

		add_free_node(n);
	} else if (distance != n->distance) {
		/* we have the same last_addr, but a shorter route */
		/* obtain new next_hop */
		n->distance = distance;
		push_default_route(n);

		add_free_node(n);
	}

	n->expires = time_now() + vtime;
}

bool is_known_msg(struct netaddr* addr, uint16_t seq_no, uint8_t vtime) {
	struct olsr_node* node = get_node(addr);
	if (!node) {
		node = _new_olsr_node(addr);
		node->seq_no = seq_no;
		node->expires = time_now() + vtime;
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
void print_topology_set(void) {
	DEBUG();
	DEBUG("---[ Topology Set ]--");

	struct olsr_node* node;
	struct alt_route* route;
	avl_for_each_element(&olsr_head, node, node) {
		DEBUG("%s (%s)\t=> %s; %d hops, next: %s, %zd s [%d] %s %s %s",
			netaddr_to_string(&nbuf[0], node->addr),
			node->name,
			netaddr_to_string(&nbuf[1], node->last_addr),
			node->distance,
			netaddr_to_string(&nbuf[2], node->next_addr),
			node->expires - time_now(),
			node->seq_no,
			node->distance != 1 ? "" : h1_deriv(node)->pending ? "pending" : "",
			node->distance != 1 ? "" : h1_deriv(node)->mpr_neigh ? "[M]" : "[ ]",
			node->distance != 1 ? "" : h1_deriv(node)->mpr_selector ? "[S]" : "[ ]"
			);
		simple_list_for_each (node->other_routes, route) {
			DEBUG("\t\t\t=> %s; %zd s",
				netaddr_to_string(&nbuf[0], route->last_addr),
				route->expires - time_now());
		}
	}
	DEBUG("---------------------");
	DEBUG();
}

void print_routing_graph(void) {
	puts("\n----BEGIN ROUTING GRAPH----\n");
	puts("subgraph routing {");
	puts("\tedge [ color = red ]");
	struct olsr_node* node, *tmp;
	avl_for_each_element(&olsr_head, node, node) {
		if (node->addr != NULL && node->last_addr != NULL) {
			tmp = get_node(node->last_addr);
			printf("\t%s -> %s\n", tmp ? tmp->name : local_name, node->name);
		}
	}
	puts("}");

	puts("subgraph mpr {");
	puts("\tedge [ color = blue ]");
	puts("// BEGIN MPR");
	avl_for_each_element(&olsr_head, node, node) {
		if (node->distance == 1 && h1_deriv(node)->mpr_selector) {
			printf("\t%s -> %s\n", node->name, local_name);
		}
	}
	puts("// END MPR");
	puts("}");

	puts("\n----END ROUTING GRAPH----\n");

}
#else
void print_topology_set(void) {}
void print_routing_graph(void) {}
#endif
