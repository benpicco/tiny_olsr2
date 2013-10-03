#include <stdlib.h>

#include "nhdp.h"
#include "util.h"
#include "debug.h"

#include "common/avl.h"
#include "common/avl_comp.h"

static struct avl_tree nhdp_2_hop_head;

struct nhdp_2_hop_node {
	struct avl_node node;

	/* node used for reaching this node */
	struct nhdp_node* mpr;

	struct netaddr* addr;
	uint8_t linkstatus;
#ifdef ENABLE_DEBUG
	char* name;
#endif
};

void nhdp_init() {
	avl_init(&nhdp_head, avl_comp_netaddr, false);
	avl_init(&nhdp_2_hop_head, avl_comp_netaddr, false);
}

struct nhdp_node* add_neighbor(struct netaddr* addr, uint8_t linkstatus) {
	struct nhdp_node* n = get_neighbor(addr);

	if (!n) {
		n = calloc(1, sizeof(struct nhdp_node));
		n->addr = netaddr_dup(addr);
		n->linkstatus = linkstatus;

		n->node.key = n->addr;
		avl_insert(&nhdp_head, &n->node);
	}

	return n;
}

int add_2_hop_neighbor(struct nhdp_node* node, struct netaddr* addr, uint8_t linkstatus, char* name) {
	struct nhdp_2_hop_node* n2;

	if (!memcmp(addr, &local_addr, sizeof local_addr))
		return -ADD_2_HOP_IS_LOCAL;

	if(avl_find_element(&nhdp_head, &addr, node, node))
		return -ADD_2HOP_IS_NEIGHBOR;

	if ((n2 = avl_find_element(&nhdp_2_hop_head, &addr, n2, node))) {
		if (n2->mpr == node || n2->mpr->mpr_neigh > node->mpr_neigh + 1)
			return -ADD_2_HOP_OK;

		DEBUG("switching MPR\n");
		n2->mpr->mpr_neigh--;
		n2->mpr = node;
		node->mpr_neigh++;
		return -ADD_2_HOP_OK;
	}

	n2 = calloc(1, sizeof(struct nhdp_2_hop_node));
	n2->mpr = node;
	n2->addr = netaddr_dup(addr);
	n2->linkstatus = linkstatus;
#ifdef ENABLE_DEBUG
	n2->name = name;
#endif
	n2->node.key = n2->addr;
	avl_insert(&nhdp_2_hop_head, &n2->node);

	node->mpr_neigh++;

	return ADD_2_HOP_OK;
}

struct nhdp_node* get_neighbor(struct netaddr* addr) {
	struct nhdp_node *n; // for typeof
	return avl_find_element(&nhdp_head, addr, n, node);
}

void remove_neighbor(struct nhdp_node* node) {
	// TODO: update mpr
	if (node) {
		avl_remove(&nhdp_head, &node->node);
		if (node->addr) {
			DEBUG("free node->addr\n"); // TODO see if this doesn't crash
			free(node->addr);
		}
		free(node);
	}
}

#ifdef ENABLE_DEBUG
void print_neighbors(void) {
	struct nhdp_node* node;

	avl_for_each_element(&nhdp_head, node, node) {
		DEBUG("neighbor: %s (%s) (mpr for %d nodes)\n",
			node->name,
			netaddr_to_string(&nbuf[0], node->addr),
			node->mpr_neigh);
	}

	struct nhdp_2_hop_node* n2;
	avl_for_each_element(&nhdp_2_hop_head, n2, node) {
		DEBUG("\t%s (%s) -> %s (%s) -> %s (%s)\n",
			n2->name, netaddr_to_string(&nbuf[0], n2->addr),
			n2->mpr->name, netaddr_to_string(&nbuf[1], n2->mpr->addr),
			node_name,
			netaddr_to_string(&nbuf[2], &local_addr));
	}
}
#else
void print_neighbors(void) {}
#endif