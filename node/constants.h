#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#define MANET_PORT	269

#define REFRESH_INTERVAL	3
#define HOLD_TIME		(3 * REFRESH_INTERVAL)
#define KEEP_EXPIRED		HOLD_TIME

#define TC_HOP_LIMIT	16

#define HYST_SCALING	0.5
#define HYST_LOW	0.3
#define HYST_HIGH	0.8

#define MAX_JITTER	500

#define FLOODING_MPR_SELECTOR 1
#define ROUTING_MPR_SELECTOR  2

#define RFC5444_TLV_NODE_NAME 42

/* NHDP message TLV array index */
enum {
	IDX_TLV_ITIME,			/* Interval time */
	IDX_TLV_VTIME,			/* validity time */
#ifdef ENABLE_DEBUG_OLSR
	IDX_TLV_NODE_NAME,		/* name of the node */
#endif
};

/* NHDP address TLV array index */
enum {
	IDX_ADDRTLV_LOCAL_IF,		/* is local if */
	IDX_ADDRTLV_LINK_STATUS,	/* link status TODO */
	IDX_ADDRTLV_MPR,		/* neighbor selected as mpr */
#ifdef ENABLE_DEBUG_OLSR
	IDX_ADDRTLV_NODE_NAME,		/* 'name' of a node from graph.gv */
#endif
};

#endif /* CONSTANTS_H_ */
