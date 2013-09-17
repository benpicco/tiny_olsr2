#include <stdlib.h>
#include <stdio.h>

#include "list.h"
#include "nhdp.h"

struct nhdp_node_2_hop {
	struct nhdp_node_2_hop* next;

	struct nhdp_node* mpr;

	struct netaddr* addr;
	uint8_t linkstatus;
#ifdef DEBUG
	char* name;
#endif
};

struct nhdp_node* n_head = 0;
struct nhdp_node* get_nn_head = 0;

struct nhdp_node_2_hop* n2_head = 0;

struct netaddr* _netaddr_cpy (struct netaddr* addr) {
	struct netaddr* addr_new = calloc(sizeof(struct netaddr), 0);
	return memcpy(addr_new, addr, sizeof(struct netaddr));
}

struct nhdp_node* add_neighbor(struct netaddr* addr, uint8_t linkstatus) {
	struct nhdp_node* new_n = list_find_memcmp(n_head, addr);

	if (!new_n) {
		new_n = list_add_head(&n_head);
		new_n->addr = _netaddr_cpy(addr);
		new_n->linkstatus = linkstatus;
	}

	return new_n;
}

int add_2_hop_neighbor(struct nhdp_node* node, struct netaddr* addr, uint8_t linkstatus, char* name) {
	struct nhdp_node_2_hop* n2_node;

	if (!memcmp(addr, &local_addr, sizeof local_addr))
		return -ADD_2_HOP_IS_LOCAL;

	if(list_find_memcmp(n_head, addr))
		return -ADD_2HOP_IS_NEIGHBOR;

	if ((n2_node = list_find_memcmp(n2_head, addr))) {
		if (n2_node->mpr == node || n2_node->mpr->mpr_neigh > node->mpr_neigh + 1)
			return -ADD_2_HOP_OK;

		printf("switching MPR\n");
		n2_node->mpr->mpr_neigh--;
		n2_node->mpr = node;
		node->mpr_neigh++;
		return -ADD_2_HOP_OK;
	}

	n2_node = list_add_head(&n2_head);
	n2_node->mpr = node;
	n2_node->addr = _netaddr_cpy(addr);
	n2_node->linkstatus = linkstatus;
#ifdef DEBUG
	n2_node->name = name;
#endif

	node->mpr_neigh++;

	return ADD_2_HOP_OK;
}

struct nhdp_node* get_neighbor(struct netaddr* addr) {
	return list_find_memcmp(n_head, addr);
}

void remove_neighbor(struct nhdp_node* node) {
	// todo: update mpr
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
	struct netaddr_str nbuf2;
	struct netaddr_str nbuf3;
	struct nhdp_node* node;

	get_next_neighbor_reset();
	while ((node = get_next_neighbor())) {
#ifdef DEBUG
		printf("neighbor: %s (%s) (mpr for %d nodes)\n", node->name, netaddr_to_string(&nbuf, node->addr), node->mpr_neigh);
#else
		printf("neighbor: %s (mpr for %d nodes)\n", netaddr_to_string(&nbuf, node->addr), node->mpr_neigh);
#endif
	}

	struct nhdp_node_2_hop* n2 = n2_head;
	while((n2)) {
#ifdef DEBUG
		printf("\t%s (%s) -> %s (%s) -> %s (%s)\n", n2->name, netaddr_to_string(&nbuf, n2->addr), n2->mpr->name, netaddr_to_string(&nbuf2, n2->mpr->addr), node_name, netaddr_to_string(&nbuf3, &local_addr));
#else
		printf("\t%s -> %s -> (O)\n", netaddr_to_string(&nbuf2, n2->addr), netaddr_to_string(&nbuf, n2->mpr->addr));
#endif
		n2 = n2->next;
	}
}
