#ifndef NHDP_H_
#define NHDP_H_

#include "common/netaddr.h"

struct netaddr node_addr;

struct nhdp_node {
	struct nhdp_node* next;
	struct nhdp_node_2_hop* hood;

	struct netaddr* addr;
	uint8_t linkstatus;
};

struct nhdp_node* add_neighbor(struct netaddr* addr, uint8_t linkstatus);

/**
* add a new neighbor of n
* may fail if n is not known
*/
int add_2_hop_neighbor(struct nhdp_node* n, struct netaddr* addr, uint8_t linkstatus);

void remove_neighbor(struct nhdp_node* n);

void get_next_neighbor_reset(void);
struct nhdp_node* get_next_neighbor(void);

void print_neighbors(void);

#endif /* NHDP_H_ */