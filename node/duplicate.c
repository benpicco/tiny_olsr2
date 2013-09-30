#include <stdlib.h>
#include <time.h>

#include "common/avl.h"
#include "common/netaddr.h"
#include "common/avl_comp.h"

#include "duplicate.h"
#include "misc.h"

struct packet {
	struct packet* next;
	uint16_t seq_no;
	time_t valid;
};

struct olsr_node {
	struct avl_node node;
	struct netaddr* addr;
	struct packet* processed;
};

static struct avl_tree olsr_head;

/**
* clears removes expired packets and returens true when seq_no is still valid
*/
bool _clear_old_packets(struct packet** head, uint16_t seq_no) {
	struct packet* node = *head;
	struct packet* prev = 0;
	bool found_seq_no	= false;
	time_t current_time = time(0);

	while (node) {
		if (node->valid < current_time) {
			if (!prev)
				*head = node->next;
			else
				prev->next = node->next;

			free(node);
			node = prev ? prev : *head;
		} else {
			if (node->seq_no == seq_no)
				found_seq_no = true;
			prev = node;
			node = node->next;
		}
	}

	return found_seq_no;
}

void _add_packet(struct packet** head, uint16_t seq_no, time_t validity) {
	struct packet* _head = *head;

	*head = malloc(sizeof(struct packet));
	(*head)->next = _head;
	(*head)->seq_no = seq_no;
	(*head)->valid = validity;
}

struct olsr_node* get_olsr_node(struct netaddr* addr) {
	struct olsr_node *n; // for typeof
	return avl_find_element(&olsr_head, addr, n, node);
}

bool is_known_msg(struct netaddr* addr, uint16_t seq_no, uint8_t validity) {
	struct olsr_node* n = get_olsr_node(addr);
	if (!n) {
		n = calloc(1, sizeof(struct olsr_node));
		n->addr = netaddr_cpy(addr);

		n->node.key = n->addr;
		avl_insert(&olsr_head, &n->node);
	}

	if (_clear_old_packets(&n->processed, seq_no))
		return true;

	_add_packet(&n->processed, seq_no, time(0) + validity);

	return false;
}

void duplicate_init() {
	avl_init(&olsr_head, avl_comp_netaddr, false);
}

