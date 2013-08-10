#ifndef NHDP_H_
#define NHDP_H_

#include "common/netaddr.h"

#ifdef DEBUG
char* node_name;
#define RFC5444_TLV_NODE_NAME 42
#endif

struct netaddr local_addr;

struct nhdp_node {
	struct nhdp_node* next;

	struct netaddr* addr;	/* node address */
	uint8_t linkstatus;		/* TODO */
	uint8_t mpr_neigh;		/* number of nodes reached by this node if it's a mpr */
#ifdef DEBUG
	char* name;				/* node name from graph.gv */
#endif
};

enum {
	ADD_2_HOP_OK,
	ADD_2_HOP_IS_LOCAL,
	ADD_2HOP_IS_NEIGHBOR
};

struct nhdp_node* add_neighbor(struct netaddr* addr, uint8_t linkstatus);

/**
* add a new neighbor of n
* may fail if n is not known
*/
int add_2_hop_neighbor(struct nhdp_node* n, struct netaddr* addr, uint8_t linkstatus, char* name);

void remove_neighbor(struct nhdp_node* n);

void get_next_neighbor_reset(void);
struct nhdp_node* get_next_neighbor(void);

void print_neighbors(void);

#endif /* NHDP_H_ */
