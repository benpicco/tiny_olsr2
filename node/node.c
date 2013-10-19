#include "node.h"
#include "list.h"
#include "debug.h"

#include "common/netaddr.h"

void node_init() {
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
	if (node->last_addr != NULL && netaddr_cmp(node->last_addr, last_addr) == 0)
		return;

	struct alt_route* route = simple_list_find_memcmp(node->other_routes, last_addr);
	if (route != NULL) {
		route->expires = time(0) + vtime;
		return;
	}

	DEBUG("adding alternative route");

	route = simple_list_add_head(&node->other_routes);
	route->last_addr = netaddr_reuse(last_addr);
	route->expires = time(0) + vtime;
}

/*
 * moves the default route of node to other_routes
 */
void push_back_default_route(struct olsr_node* node) {
	struct netaddr* last_addr = node->last_addr;
	struct alt_route* route = simple_list_find_memcmp(node->other_routes, last_addr);
	if (route != NULL) {
		node->last_addr = netaddr_free(node->last_addr);
		return;
	}

	route = simple_list_add_head(&node->other_routes);
	route->expires = node->expires;
	route->last_addr = node->last_addr;
	route->last_addr = NULL;
}

void remove_other_route(struct olsr_node* node, struct netaddr* last_addr) {
	struct alt_route *route, *prev;
	simple_list_for_each_safe(node->other_routes, route, prev) {
		if (netaddr_cmp(route->last_addr, last_addr))
			continue;

		netaddr_free(route->last_addr);
		simple_list_for_each_remove(&node->other_routes, route, prev);
		break;
	}
}