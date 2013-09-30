#include <stdlib.h>
#include "common/netaddr.h"
#include "misc.h"

struct netaddr* netaddr_cpy (struct netaddr* addr) {
	struct netaddr* addr_new = calloc(sizeof(struct netaddr), 0);
	return memcpy(addr_new, addr, sizeof(struct netaddr));
}
