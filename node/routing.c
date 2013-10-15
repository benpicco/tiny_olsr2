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

void add_free_node(struct free_node** head, struct olsr_node* node) {
	struct free_node* n = simple_list_find_cmp(*head, node, olsr_node_cmp);
	if (n == NULL)
		n = simple_list_add_before(head, node->distance);

	n->hops = node->distance;
	n->node = node;
}

void remove_free_node(struct free_node** head, struct olsr_node* node) {
	struct free_node* n = simple_list_find_cmp(*head, node, olsr_node_cmp);
	if (n == NULL)
		return;
	simple_list_remove(head, n);
}

void fill_routing_table(struct free_node** head) {
	struct free_node* _head = *head;

	struct olsr_node* node;
	struct free_node* fn;
	bool noop = false;	/* when in an iteration there was nothing remove from free nodes */
	bool delete = false;
	while (_head && !noop) {
		noop = true;	/* if no nodes could be removed in an iteration, abort */
		struct free_node *tmp, *prev = 0;

		simple_list_for_each (_head, fn) {
start:
			DEBUG("simple_list_for_each iteration (%p)", fn);
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
					_head = *head = _head->next;
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
	while(_head) {
		DEBUG("Could not find next hop for %s (%s), should be %s (%d hops)",
			netaddr_to_string(&nbuf[0], _head->node->addr), _head->node->name,
			netaddr_to_string(&nbuf[1], _head->node->last_addr), _head->node->distance);

		_head = _head->next;
	}
#endif
}
