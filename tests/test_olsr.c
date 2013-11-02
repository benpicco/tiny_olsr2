#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include "node.h"
#include "nhdp.h"
#include "olsr.h"

#include "cunit/cunit.h"
#include "common/netaddr.h"

static char this_ip[] = "2001::1";

static struct netaddr addrs[] = {
	{ {0x20,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,2 }, AF_INET6, 128 },
	{ {0x20,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,3 }, AF_INET6, 128 },
	{ {0x20,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,4 }, AF_INET6, 128 },
	{ {0x20,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,5 }, AF_INET6, 128 },
	{ {0x20,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,6 }, AF_INET6, 128 }
};

static bool is_next_hop(uint8_t a, uint8_t b) {
	struct olsr_node* an = get_node(&addrs[a]);
	struct olsr_node* bn = get_node(&addrs[b]);

	return netaddr_cmp(bn->addr, an->next_addr) == 0;
}

static bool is_last_hop(uint8_t a, uint8_t b) {
	struct olsr_node* an = get_node(&addrs[a]);
	struct olsr_node* bn = get_node(&addrs[b]);

	return netaddr_cmp(bn->addr, an->last_addr) == 0;
}

static bool is_mpr_neigh(uint8_t a, uint8_t count) {
	struct olsr_node* n = get_node(&addrs[a]);
	if (n->type != NODE_TYPE_NHDP) {
		puts("ERROR: wrong node type");
		return false;
	}

	return h1_deriv(n)->mpr_neigh == count;
}

static void test_add_neighbors() {
	add_neighbor(&addrs[0], 5, 0);
	add_olsr_node(&addrs[1], &addrs[0], 5, 1, 0);

	remove_expired(0);
	remove_expired(0);
	remove_expired(0);

	add_olsr_node(&addrs[2], &addrs[1], 5, 1, 0);
	remove_expired(&addrs[2]);

	print_topology_set();

	START_TEST();

	CHECK_TRUE(is_last_hop(1, 0), "is_last_hop(%u, %u)", 1, 0);
	CHECK_TRUE(is_next_hop(1, 0), "is_next_hop(%u, %u)", 1, 0);
	CHECK_TRUE(is_next_hop(2, 0), "is_next_hop(%u, %u)", 2, 0);
	CHECK_TRUE(is_mpr_neigh(0, 1), "mpr_neigh(%u) != %u", 0, 1);

	END_TEST();
}

int main(void) {

	node_init();
	get_local_addr()->_type = AF_INET6;
	get_local_addr()->_prefix_len = 128;
	inet_pton(AF_INET6, this_ip, get_local_addr()->_addr);

	BEGIN_TESTING(0);

	test_add_neighbors();

	return FINISH_TESTING();
}
