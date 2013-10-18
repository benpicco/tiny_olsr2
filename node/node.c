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

void add_other_route(struct olsr_node* node, uint8_t hops, struct netaddr* last_addr, uint8_t vtime) {
	DEBUG("add_other_route(%s, %d, %s)", 
		netaddr_to_string(&nbuf[0], node->addr),
		hops,
		netaddr_to_string(&nbuf[1], last_addr));

	if (netaddr_cmp(node->last_addr, last_addr) == 0)
		return;

	struct alt_route* route = simple_list_find_memcmp(node->other_routes, last_addr);

	if (route != NULL) {
		DEBUG("updating alternative route");
		if (hops > route->hops)
			return;

		route->expires = time(0) + vtime;
		route->hops = hops;
		return;
	}

	DEBUG("adding alternative route");
	route = simple_list_add_before(&node->other_routes, hops);
	route->last_addr = netaddr_reuse(last_addr);
	route->hops = hops;
	route->expires = time(0) + vtime;
}

void remove_other_route(struct olsr_node* node, struct netaddr* last_addr) {
	struct alt_route* route = simple_list_find_memcmp(node->other_routes, last_addr);
	if (route != NULL)
		simple_list_remove(&node->other_routes, route);
}