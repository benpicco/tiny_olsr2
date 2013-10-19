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
	time_t expires;				/* time when this tuple is invalid */
	uint16_t seq_no;			/* last seq_no from last_addr */
	uint8_t distance;			/* hops between us and the node */
	struct netaddr* next_addr;	/* neighbor addr to send packets to for this node*/
	struct netaddr* last_addr;	/* node that announced this node */

	struct alt_route* other_routes;

#ifdef ENABLE_DEBUG
	char* name;					/* node name from graph.gv */
#endif
};

struct nhdp_node {
	struct olsr_node super;

	float link_quality;			/* the quality of the link */
	uint8_t mpr_neigh;			/* number of nodes reached by this node if it's an MPR */
	uint8_t mpr_selector;		/* wheather the node selected us as an MPR */
	uint8_t pending;			/* wheather the link can already be used */
};

static inline struct olsr_node* h1_super(struct nhdp_node* n)		{ return (struct olsr_node*) n; }
static inline struct nhdp_node* h1_deriv(struct olsr_node* n) {
	if (n == NULL)
		return 0;

	if (n->distance != 1)
		return 0;

	return (struct nhdp_node*) n;
}

void node_init();
int olsr_node_cmp(struct olsr_node* a, struct olsr_node* b);
struct olsr_node* get_node(struct netaddr* addr);

void add_other_route(struct olsr_node* node, struct netaddr* last_addr, uint8_t vtime);
void remove_other_route(struct olsr_node* node, struct netaddr* last_addr);
void push_default_route(struct olsr_node* node);
void pop_other_route(struct olsr_node* node, struct netaddr* last_addr);

#endif /* NODE_H_ */