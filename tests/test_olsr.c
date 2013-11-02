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

static void test_add_neighbors() {
	add_neighbor(&addrs[0], 5, 0);
	add_olsr_node(&addrs[1], &addrs[0], 5, 1, 0);

	remove_expired(0);
	remove_expired(0);
	remove_expired(0);

	add_olsr_node(&addrs[2], &addrs[1], 5, 1, 0);

	print_topology_set();
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
