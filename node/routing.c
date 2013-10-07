#include <stdlib.h>

#include "olsr.h"
#include "list.h"
#include "debug.h"
#include "util.h"
#include "routing.h"

/* sorted list, only for faster access */
struct free_node {
	struct free_node* next;
	struct olsr_node* node;
	int hops;
};

int olsr_node_cmp(struct olsr_node* a, struct olsr_node* b) {
	return netaddr_cmp(a->addr, b->addr);
}

void add_free_node(struct free_node** head, struct olsr_node* node) {
	struct free_node* n = simple_list_find_cmp(*head, node->addr, olsr_node_cmp);
	if (!n)
		n = simple_list_add_before(head, node->distance);
	n->hops = node->distance;
	n->node = node;
}

void fill_routing_table(struct free_node** head) {
	struct free_node* _head = *head;

	struct olsr_node* node;
	struct free_node* fn;
	int hops = 2;
	bool noop = false;
	while (_head && !noop) {
		noop = true;	/* if no nodes could be removed in an iteration, abort */
		struct free_node *tmp, *prev = 0;

		++hops;
		DEBUG("Adding nodes with distance %d", hops);

		simple_list_for_each (_head, fn) {
start:
			DEBUG("simple_list_for_each iteration (%p)", fn);
			if ((node = get_node(fn->node->last_addr))) {
				noop = false;
				DEBUG("%s (%s) -> %s (%s) -> […] -> %s",
					netaddr_to_string(&nbuf[0], fn->node->addr), fn->node->name,
					netaddr_to_string(&nbuf[1], node->addr), node->name,
					netaddr_to_string(&nbuf[2], node->next_addr));

				if (node->next_addr) {
					fn->node->next_addr = netaddr_use(node->next_addr);
					DEBUG("%d = %d", fn->node->distance, hops);
					fn->node->distance = hops;

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
				} else
					DEBUG("Don't know how to route this one yet.");
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