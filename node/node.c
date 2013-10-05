#include "node.h"

void node_init() {
	avl_init(&olsr_head, avl_comp_netaddr, false);
}

struct olsr_node* get_node(struct netaddr* addr) {
	struct olsr_node *n; // for typeof
	return avl_find_element(&olsr_head, addr, n, node);
}
