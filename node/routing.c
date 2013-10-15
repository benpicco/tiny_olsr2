#include <stdlib.h>

#include "olsr.h"
#include "list.h"
#include "debug.h"
#include "util.h"
#include "routing.h"

/* sorted list, only for faster access
 * Keeps yet unroutable nodes, so we don't have to traverse the entire list
 */
struct free_node {
	struct free_node* next;
	struct olsr_node* node;
	int hops;
};

struct free_node* _head = 0;

void add_free_node(struct olsr_node* node) {
	struct free_node* n = simple_list_find_cmp(_head, node, olsr_node_cmp);
	if (n == NULL)
		n = simple_list_add_before(&_head, node->distance);

	n->hops = node->distance;
	n->node = node;
}

void remove_free_node(struct olsr_node* node) {
	struct free_node* n = simple_list_find_cmp(_head, node, olsr_node_cmp);
	if (n == NULL)
		return;
	simple_list_remove(&_head, n);
}

bool pending_nodes_exist(void) {
	return _head != NULL;
}

void fill_routing_table(void) {
	struct free_node* __head = _head;

	struct olsr_node* node;
	struct free_node* fn;
	bool noop = false;	/* when in an iteration there was nothing remove from free nodes */
	bool delete = false;
	while (__head && !noop) {
		noop = true;	/* if no nodes could be removed in an iteration, abort */
		struct free_node *tmp, *prev = 0;

		simple_list_for_each (__head, fn) {
start:
			DEBUG("simple_list_for_each iteration (%p) - %s", fn, fn->node->name);
			DEBUG("addr: %s\tlast_addr: %s\thops: %d\t%d s",
				netaddr_to_string(&nbuf[0], fn->node->addr),
				netaddr_to_string(&nbuf[1], fn->node->next_addr),
				fn->node->distance,
				fn->node->expires - time(0));
			/* remove expired nodes */
			if (fn->node->expires < time(0)) {
				delete = true;
				DEBUG("%s expired in free_node list", fn->node->name);
			} else
			/* get next hop */
			if ((node = get_node(fn->node->last_addr))) {
				DEBUG("%s (%s) -> %s (%s) -> […] -> %s",
					netaddr_to_string(&nbuf[0], fn->node->addr), fn->node->name,
					netaddr_to_string(&nbuf[1], node->addr), node->name,
					netaddr_to_string(&nbuf[2], node->next_addr));

				if (node->next_addr) {
					noop = false;

					fn->node->next_addr = netaddr_use(node->next_addr);
					DEBUG("%d = %d", fn->node->distance, node->distance + 1);
					fn->node->distance = node->distance + 1;

					assert(is_valid_neighbor(fn->node->addr, fn->node->last_addr));

					delete = true;
				} else
					DEBUG("Don't know how to route this one yet.");
			}

			if (delete) {
				delete = false;
				/* remove free node */
				tmp = fn->next;
				if (!prev)
					__head = _head = __head->next;
				else
					prev->next = fn->next;

				free(fn);
				if ((fn = tmp))
					goto start;
				else
					break;
			}

			prev = fn;
		}
	}

#ifdef DEBUG
	while(__head) {
		DEBUG("Could not find next hop for %s (%s), should be %s (%d hops)",
			netaddr_to_string(&nbuf[0], __head->node->addr), __head->node->name,
			netaddr_to_string(&nbuf[1], __head->node->last_addr), __head->node->distance);

		__head = __head->next;
	}
#endif
}
