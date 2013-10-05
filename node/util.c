#include <stdlib.h>
#include "util.h"

struct netaddr* netaddr_dup(struct netaddr* addr) {
	struct netaddr_rc* addr_new = calloc(1, sizeof(struct netaddr_rc));
	addr_new->_refs = 1;
	return memcpy(addr_new, addr, sizeof(struct netaddr));
}

struct netaddr* netaddr_use(struct netaddr* addr) {
	((struct netaddr_rc*) addr)->_refs++;
	return addr;
}

struct netaddr* netaddr_free(struct netaddr* addr) {
	struct netaddr_rc* addr_rc = (struct netaddr_rc*) addr;
	if (--addr_rc->_refs == 0) {
		free(addr_rc);
		return 0;
	}
	return addr;
}
