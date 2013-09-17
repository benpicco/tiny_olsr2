#ifndef CONSTANTS_H_
#define CONSTANTS_H_

// todo: sensible values
#define REFRESH_INTERVAL 5
#define HOLD_TIME 10

#define RFC5444_TLV_NODE_NAME 42

/* NHDP message TLV array index */
enum {
  IDX_TLV_ITIME,			/* Interval time */
  IDX_TLV_VTIME,			/* validity time */
  IDX_TLV_WILLINGNESS,		/* willingness - split out to FLOOD and ROUTING? */
#ifdef DEBUG
  IDX_TLV_NODE_NAME,		/* name of the node */
#endif
};

/* NHDP address TLV array index */
enum {
  IDX_ADDRTLV_LOCAL_IF,     /* is local if */
  IDX_ADDRTLV_LINK_STATUS,  /* link status TODO */
  IDX_ADDRTLV_MPR,          /* neighbor selected as mpr */
  IDX_ADDRTLV_LINKMETRIC,   /* link metric TODO */
#ifdef DEBUG
  IDX_ADDRTLV_NODE_NAME,    /* 'name' of a node from graph.gv */
#endif
};

/* TC message TLV array index */
enum {
  IDX_ADDRTLV_NBR_ADDR_TYPE,
  IDX_ADDRTLV_GATEWAY,
};

#endif /* CONSTANTS_H_ */
