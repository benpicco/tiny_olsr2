#ifndef NHDP_H_
#define NHDP_H_

#ifdef RIOT
#include <sixlowpan/sixlowip.h>
#else
#include <netinet/in.h>
typedef struct in6_addr ipv6_addr_t;
#endif

struct nhdp_node {
	ipv6_addr_t addr;
};

void add_neighbor(struct nhdp_node* n);

/**
* add m as a neighbor of n
* may fail if n is not known
*/
int add_2_hop_neighbor(struct nhdp_node* n, struct nhdp_node* m);

void remove_neighbor(struct nhdp_node* n);

void get_next_neighbor_reset(void);
struct nhdp_node* get_next_neighbor(void);

#endif /* NHDP_H_ */