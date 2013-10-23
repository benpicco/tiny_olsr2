#ifndef NODE_H_
#define NODE_H_

#include "common/avl.h"
#include "common/avl_comp.h"
#include "common/netaddr.h"

#include "util.h"
#include "debug.h"

struct netaddr_rc _local_addr;
struct netaddr* local_addr;

#ifdef ENABLE_DEBUG
char* local_name;
#endif

enum {
	NODE_TYPE_OLSR,
	NODE_TYPE_NHDP
};

struct avl_tree olsr_head;

/* simple list to store alternative routes */
struct alt_route {
	struct alt_route* next;

	struct netaddr* last_addr;
	time_t expires;
};

struct olsr_node {
	struct avl_node node;		/* for routing table Information Base */

	struct netaddr* addr;		/* node address */
	struct netaddr* next_addr;	/* neighbor addr to send packets to for this node*/
	struct netaddr* last_addr;	/* node that announced this node */
	struct alt_route* other_routes; /* other possible last_addrs */

	time_t expires;				/* time when this tuple is invalid */
	uint16_t seq_no;			/* last seq_no from last_addr */
	uint8_t distance;			/* hops between us and the node */

	uint8_t type;				/* node type */

#ifdef ENABLE_DEBUG
	char* name;					/* node name from graph.gv */
#endif
};

struct nhdp_node {
	struct olsr_node super;

	uint8_t mpr_neigh;			/* number of 2-hop neighbors reached through this node 
								   aka if this value is > 0, it's a MPR */
	uint8_t mpr_selector;		/* whether the node selected us as a MPR */
	uint8_t pending;			/* whether the link can already be used */
	float link_quality;			/* average packet loss, decides if it should be used as 1-hop neigh */
};

static inline struct olsr_node* h1_super(struct nhdp_node* n)		{ return (struct olsr_node*) n; }
static inline struct nhdp_node* h1_deriv(struct olsr_node* n) {
	if (n == NULL)
		return 0;

	if (n->type != NODE_TYPE_NHDP)
		return 0;

	return (struct nhdp_node*) n;
}

void node_init(void);
int olsr_node_cmp(struct olsr_node* a, struct olsr_node* b);
struct olsr_node* get_node(struct netaddr* addr);

void add_other_route(struct olsr_node* node, struct netaddr* last_addr, uint8_t vtime);
void remove_other_route(struct olsr_node* node, struct netaddr* last_addr);
void remove_default_node(struct olsr_node* node);
void push_default_route(struct olsr_node* node);
void pop_other_route(struct olsr_node* node, struct netaddr* last_addr);

#endif /* NODE_H_ */
