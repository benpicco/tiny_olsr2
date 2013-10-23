#include "node.h"
#include "list.h"
#include "debug.h"

#include "common/netaddr.h"

int olsr_node_cmp(struct olsr_node* a, struct olsr_node* b) {
	return netaddr_cmp(a->addr, b->addr);
}

void node_init(void) {
	_local_addr._refs = 1;
	local_addr = (struct netaddr*) &_local_addr;
	avl_init(&olsr_head, avl_comp_netaddr, false);
}

struct olsr_node* get_node(struct netaddr* addr) {
	struct olsr_node *n; // for typeof
	if (addr == NULL)
		return NULL;
	return avl_find_element(&olsr_head, addr, n, node);
}

void add_other_route(struct olsr_node* node, struct netaddr* last_addr, uint8_t vtime) {
	DEBUG("add_other_route(%s, %s)", 
		netaddr_to_string(&nbuf[0], node->addr),
		netaddr_to_string(&nbuf[1], last_addr));

	/* make sure the route is not already the default route */
	if (node->last_addr != NULL && netaddr_cmp(node->last_addr, last_addr) == 0) {
		node->expires = time_now() + vtime;
		return;
	}

	struct alt_route* route = simple_list_find_memcmp(node->other_routes, last_addr);
	if (route != NULL) {
		route->expires = time_now() + vtime;
		return;
	}

	route = simple_list_add_head(&node->other_routes);
	route->last_addr = netaddr_reuse(last_addr);
	route->expires = time_now() + vtime;
}

void _decrease_mpr_neigh(struct olsr_node* node) {
	/* update MPR information */
	if (node->distance == 2) {
		struct nhdp_node* n1 = h1_deriv(get_node(node->last_addr));
		if (n1 != NULL && n1->mpr_neigh > 0)
			n1->mpr_neigh--;
	}
}

void remove_default_node(struct olsr_node* node) {
	if (node->last_addr) {
		_decrease_mpr_neigh(node);
		node->last_addr = netaddr_free(node->last_addr);
	}
	node->next_addr = netaddr_free(node->next_addr);
}

/*
 * moves the default route of node to other_routes
 */
void push_default_route(struct olsr_node* node) {
	struct netaddr* last_addr = node->last_addr;

	if (node->last_addr == NULL)
		return;

	_decrease_mpr_neigh(node);
	struct alt_route* route = simple_list_find_memcmp(node->other_routes, last_addr);
	if (route != NULL) {
		node->last_addr = netaddr_free(node->last_addr);
		return;
	}

	route = simple_list_add_head(&node->other_routes);
	route->expires = node->expires;
	route->last_addr = node->last_addr;
	node->last_addr = NULL;
}

void pop_other_route(struct olsr_node* node, struct netaddr* last_addr) {
	char skipped;
	struct alt_route *route, *prev;
	simple_list_for_each_safe(node->other_routes, route, prev, skipped) {
		if (netaddr_cmp(route->last_addr, last_addr))
			continue;

		node->last_addr = route->last_addr;
		node->expires = route->expires;
		simple_list_for_each_remove(&node->other_routes, route, prev);
		break;
	}
}

void remove_other_route(struct olsr_node* node, struct netaddr* last_addr) {
	char skipped;
	struct alt_route *route, *prev;
	simple_list_for_each_safe(node->other_routes, route, prev, skipped) {
		if (netaddr_cmp(route->last_addr, last_addr))
			continue;

		netaddr_free(route->last_addr);
		simple_list_for_each_remove(&node->other_routes, route, prev);
		break;
	}
}
