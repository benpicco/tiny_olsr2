#include <stdlib.h>

#include "list.h"
#include "nhdp.h"

struct neighbor_2_hop {
	struct neighbor_2_hop* next;
	struct nhdp_node* node;
};

struct neighbor {
	struct neighbor* next;
	struct nhdp_node* node;
	struct neighbor_2_hop* hood;
};

struct neighbor* n_head = 0;
struct neighbor* get_nn_head = 0;

static struct neighbor* _get_neighbor_by_node(struct neighbor* head, struct nhdp_node* node) {
	while (head) {
		if (head->node == node)
			return head;
		head = head->next;
	}

	return 0;
}

void add_neighbor(struct nhdp_node* n) {
	struct neighbor* new_n = add_tail((struct list_elem*) n_head, sizeof(struct neighbor));
	new_n->node = n;
}

int add_2_hop_neighbor(struct nhdp_node* n, struct nhdp_node* m) {
	struct neighbor* node = _get_neighbor_by_node(n_head, n);
	if (node) {
		struct neighbor_2_hop* new_n = add_tail((struct list_elem*) node->hood, sizeof(struct neighbor_2_hop));
		new_n->node = m;
	}
	return 0;
}

void remove_neighbor(struct nhdp_node* n) {
	struct neighbor* node = _get_neighbor_by_node(n_head, n);
	if (node)
		list_remove((struct list_elem*) n_head,(struct list_elem*)  node);
}

void get_next_neighbor_reset(void) {
	get_nn_head = n_head;
}

struct nhdp_node* get_next_neighbor(void) {
	struct nhdp_node* current_node = 0;

	if (get_nn_head) {
		current_node = get_nn_head->node;
		get_nn_head = get_nn_head->next;
	}
	
	return current_node;
}