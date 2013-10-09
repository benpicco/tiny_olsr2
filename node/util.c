#include <stdlib.h>
#include "util.h"
#include "node.h"
#include "debug.h"

struct netaddr* netaddr_dup(struct netaddr* addr) {
	struct netaddr_rc* addr_new = calloc(1, sizeof(struct netaddr_rc));
	addr_new->_refs = 1;
	return memcpy(addr_new, addr, sizeof(struct netaddr));
}

struct netaddr* netaddr_use(struct netaddr* addr) {
	((struct netaddr_rc*) addr)->_refs++;
	return addr;
}

struct netaddr* netaddr_reuse(struct netaddr* addr) {
	struct olsr_node* n = get_node(addr);
	if (!n) {
		DEBUG("Address %s not found, this shouldn't happen", netaddr_to_string(&nbuf[0], addr));
		return netaddr_dup(addr);
	}
	return n->addr;
}

struct netaddr* netaddr_free(struct netaddr* addr) {
	struct netaddr_rc* addr_rc = (struct netaddr_rc*) addr;
	if (addr != NULL && --addr_rc->_refs == 0) {
		free(addr_rc);
		return 0;
	}
	return addr;
}
