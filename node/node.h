#ifndef NODE_H_
#define NODE_H_

#include <assert.h>

#include "common/avl.h"
#include "common/avl_comp.h"
#include "common/netaddr.h"

struct avl_tree olsr_head;

enum {
	NODE_TYPE_OLSR,
	NODE_TYPE_1_HOP,
	NODE_TYPE_2_HOP
};

struct olsr_node {
	struct avl_node node;		/* for routing table Information Base */
	uint8_t type;				/* node type from enum above */

	struct netaddr* addr;		/* node address */
	time_t expires;				/* time when this tuple is invalid */
	uint16_t seq_no;			/* last seq_no from last_addr */

	uint8_t distance;			/* hops between us and the node */
	struct netaddr* next_addr;	/* neighbor addr to send packets to for this node*/
	struct netaddr* last_addr;	/* node that announced this node */

#ifdef ENABLE_DEBUG
	char* name;					/* node name from graph.gv */
#endif
};

struct nhdp_node {
	struct olsr_node super;

	uint8_t linkstatus;			/* wheather we have a symetric link */
	uint8_t mpr_neigh;			/* number of nodes reached by this node if it's an MPR */
	uint8_t mpr_selector;		/* wheather the node selected us as an MPR */
};

struct nhdp_2_hop_node {
	struct olsr_node super;

	uint8_t linkstatus;
};

static inline struct olsr_node* h1_super(struct nhdp_node* n)		{ return (struct olsr_node*) n; }
static inline struct olsr_node* h2_super(struct nhdp_2_hop_node* n)	{ return (struct olsr_node*) n; }
static inline struct nhdp_node* h1_deriv(struct olsr_node* n) {
	assert(n);
	assert(n->distance == 1);
	assert(n->type == NODE_TYPE_1_HOP);
	return (struct nhdp_node*) n;
}
static inline struct nhdp_2_hop_node* h2_deriv(struct olsr_node* n) {
	assert(n);
	assert(n->distance == 2);
	assert(n->type == NODE_TYPE_2_HOP);
	return (struct nhdp_2_hop_node*) n;
}

void node_init();
struct olsr_node* get_node(struct netaddr* addr);

#endif