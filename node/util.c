#include <stdlib.h>
#include "util.h"

struct netaddr* netaddr_dup(struct netaddr* addr) {
	struct netaddr* addr_new = calloc(1, sizeof(struct netaddr));
	return memcpy(addr_new, addr, sizeof(struct netaddr));
}

