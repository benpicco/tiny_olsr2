#include <stdio.h>
#include <stdlib.h>

#include "common/avl.h"
#include "common/avl_comp.h"
#include "common/netaddr.h"

struct olsr_node {
	struct netaddr addr;
	const char* name;

	struct avl_node node;
};

static struct avl_tree olsr_head;

void add_node(struct netaddr* addr, const char* name) {
	struct olsr_node* node = calloc(1, sizeof(struct olsr_node));

	memcpy(&node->addr, addr, sizeof(struct netaddr));
	node->name = name;

	node->node.key = &node->addr;
	avl_insert(&olsr_head, &node->node);
}

int main() {
	struct netaddr addr;
	avl_init(&olsr_head, avl_comp_netaddr, false);

	netaddr_from_string(&addr, "2001::1");
	add_node(&addr, "A");

	netaddr_from_string(&addr, "2001::2");
	add_node(&addr, "B");

	add_node(&addr, "C");

	struct olsr_node *node;
	struct netaddr_str nbuf;
	avl_for_each_element(&olsr_head, node, node) {
		printf("%s - %s\n", node->name, netaddr_to_str_s(&nbuf, &node->addr));
	}

	netaddr_from_string(&addr, "2001::1");
	if ((node = avl_find_element(&olsr_head, &addr, node, node)))
		printf("Found: %s\n", node->name);
	else
		printf("Not found.\n");

	return 0;
};
