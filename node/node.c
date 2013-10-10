#include "node.h"

void node_init() {
	_local_addr._refs = 1;
	local_addr = (struct netaddr*) &_local_addr;
	avl_init(&olsr_head, avl_comp_netaddr, false);
}

struct olsr_node* get_node(struct netaddr* addr) {
	struct olsr_node *n; // for typeof
	return avl_find_element(&olsr_head, addr, n, node);
}
