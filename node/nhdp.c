#include <stdlib.h>
#include <stdio.h>

#include "list.h"
#include "nhdp.h"

struct nhdp_node_2_hop {
	struct neighbor_2_hop* next;

	struct netaddr* addr;
	uint8_t linkstatus;
};

struct nhdp_node* n_head = 0;
struct nhdp_node* get_nn_head = 0;

struct nhdp_node* add_neighbor(struct netaddr* addr, uint8_t linkstatus) {
	struct nhdp_node* new_n = list_find_memcmp(n_head, addr);
	if (!new_n) {
		new_n = list_add_head(&n_head);
		new_n->addr = malloc(sizeof(struct netaddr));
		memcpy(new_n->addr, addr, sizeof(struct netaddr));
		new_n->linkstatus = linkstatus;
	}

	return new_n;
}

int add_2_hop_neighbor(struct nhdp_node* node, struct netaddr* addr, uint8_t linkstatus) {
	struct nhdp_node* n = list_find(n_head, addr);
	if (n) {
		struct nhdp_node_2_hop* new_m = list_add_head(&n->hood);
		new_m->addr = addr;
		new_m->linkstatus = linkstatus;
	}
	return 0;
}

void remove_neighbor(struct nhdp_node* node) {
	if (node)
		list_remove(&n_head, node);
}

void get_next_neighbor_reset(void) {
	get_nn_head = n_head;
}

struct nhdp_node* get_next_neighbor(void) {
	struct nhdp_node* ret = get_nn_head;

	if (get_nn_head) {
		get_nn_head = get_nn_head->next;
	}
	
	return ret;
}

void print_neighbors(void) {
	struct netaddr_str nbuf;

	struct nhdp_node* node;
	get_next_neighbor_reset();
	while ((node = get_next_neighbor())) {
		printf("%s\n", netaddr_to_string(&nbuf, node->addr));
	}
}