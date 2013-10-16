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

struct free_node* _pending_head = 0;
bool _update_pending = false;

void add_free_node(struct olsr_node* node) {
	struct free_node* n = simple_list_find_cmp(_pending_head, node, (int (*)(void *, void *)) olsr_node_cmp);
	if (n == NULL)
		n = simple_list_add_before(&_pending_head, node->distance);

	n->hops = node->distance;
	n->node = node;
	_update_pending = true;
}

void remove_free_node(struct olsr_node* node) {
	struct free_node* n = simple_list_find_cmp(_pending_head, node, (int (*)(void *, void *)) olsr_node_cmp);
	if (n == NULL)
		return;
	simple_list_remove(&_pending_head, n);
}

void sched_routing_update(void) {
	_update_pending = true;
}

void fill_routing_table(void) {
	struct free_node* head = _pending_head;

	if (_pending_head == NULL || !_update_pending)
		return;

	_update_pending = false;
	DEBUG("update routing table");

	struct olsr_node* node;
	struct free_node* fn;
	bool noop = false;	/* when in an iteration there was nothing remove from free nodes */
	bool delete = false;
	while (head && !noop) {
		noop = true;	/* if no nodes could be removed in an iteration, abort */
		struct free_node *tmp, *prev = 0;

		simple_list_for_each (head, fn) {
start:
			DEBUG("simple_list_for_each iteration (%p) - %s", fn, fn->node->name);
			DEBUG("addr: %s\tlast_addr: %s\thops: %d\t%ld s",
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
					_pending_head = head = head->next;
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
	while (head != NULL) {
		DEBUG("Could not find next hop for %s (%s), should be %s (%d hops)",
			netaddr_to_string(&nbuf[0], head->node->addr), head->node->name,
			netaddr_to_string(&nbuf[1], head->node->last_addr), head->node->distance);

		head = head->next;
	}
#endif
}
