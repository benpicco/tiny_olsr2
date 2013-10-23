#include <stdlib.h>

#ifdef RIOT
#include "vtimer.h"
#else
#include <time.h>
#endif

#include "util.h"
#include "node.h"
#include "debug.h"

const char* netaddr_to_str_s(struct netaddr_str* dst, const struct netaddr* src) {
	return src ? netaddr_to_string(dst, src) : NULL;
}

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
	if (netaddr_cmp(addr, local_addr) == 0)
		return netaddr_use(local_addr);

	struct olsr_node* n = get_node(addr);
	if (!n) {
		DEBUG("Address %s not found, this shouldn't happen", netaddr_to_str_s(&nbuf[0], addr));
		return netaddr_dup(addr);
	}
	return netaddr_use(n->addr);
}

struct netaddr* netaddr_free(struct netaddr* addr) {
	struct netaddr_rc* addr_rc = (struct netaddr_rc*) addr;

	if (addr)
		DEBUG("netaddr_free(%s) - %d refs", netaddr_to_str_s(&nbuf[0], addr), addr_rc->_refs);

	if (addr != NULL && --addr_rc->_refs == 0)
		free(addr_rc);

	return NULL;
}

time_t time_now(void) {
#ifdef RIOT
	timex_t now;
	vtimer_now(&now);
	return now.seconds;
#else
	return time(0);
#endif
}
