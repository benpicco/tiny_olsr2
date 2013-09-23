#include <string.h>
#include <stdio.h>

#ifdef DEBUG
#include <string.h>
#endif

#ifdef RIOT
#include "net_help/net_help.h"
#endif

#include "common/avl.h"
#include "common/common_types.h"
#include "common/netaddr.h"
#include "rfc5444/rfc5444.h"
#include "rfc5444/rfc5444_iana.h"
#include "rfc5444/rfc5444_writer.h"

#include "writer.h"
#include "nhdp.h"
#include "constants.h"

uint8_t msg_buffer[128];
uint8_t msg_addrtlvs[1000];
uint8_t packet_buffer[128];

struct rfc5444_writer_target interface;

struct rfc5444_writer writer;

static void _cb_add_nhdp_message_TLVs(struct rfc5444_writer *wr);
static void _cb_add_nhdp_addresses(struct rfc5444_writer *wr);

static void _cb_add_olsr_message_TLVs(struct rfc5444_writer *wr);
static void _cb_add_olsr_addresses(struct rfc5444_writer *wr);

static struct rfc5444_writer_content_provider _nhdp_message_content_provider = {
	.msg_type = RFC5444_MSGTYPE_HELLO,
	.addMessageTLVs = _cb_add_nhdp_message_TLVs,
	.addAddresses = _cb_add_nhdp_addresses,
};

static struct rfc5444_writer_tlvtype _nhdp_addrtlvs[] = {
  [IDX_ADDRTLV_LOCAL_IF] = { .type = RFC5444_ADDRTLV_LOCAL_IF },
  [IDX_ADDRTLV_LINK_STATUS] =  { .type = RFC5444_ADDRTLV_LINK_STATUS },
  [IDX_ADDRTLV_MPR] = { .type = RFC5444_ADDRTLV_MPR },
  [IDX_ADDRTLV_LINKMETRIC] = { .type = RFC5444_ADDRTLV_LINK_METRIC },
#ifdef DEBUG
  [IDX_ADDRTLV_NODE_NAME] = { .type = RFC5444_TLV_NODE_NAME },
#endif
};

static struct rfc5444_writer_content_provider _olsr_message_content_provider = {
  .msg_type = RFC5444_MSGTYPE_TC,
  .addMessageTLVs = _cb_add_olsr_message_TLVs,
  .addAddresses = _cb_add_olsr_addresses,
};

static struct rfc5444_writer_tlvtype _olsr_addrtlvs[] = {
#ifdef DEBUG
  [IDX_ADDRTLV_NODE_NAME] = { .type = RFC5444_TLV_NODE_NAME },
#endif
};

/**
 * Callback to add message TLVs to a RFC5444 message
 * @param wr
 */
static void
_cb_add_nhdp_message_TLVs(struct rfc5444_writer *wr) {
	uint8_t time_encoded;

	time_encoded = rfc5444_timetlv_encode(REFRESH_INTERVAL);
  	rfc5444_writer_add_messagetlv(wr, RFC5444_MSGTLV_VALIDITY_TIME, 0,
    	&time_encoded, sizeof(time_encoded));

  	time_encoded = rfc5444_timetlv_encode(HOLD_TIME);
  	rfc5444_writer_add_messagetlv(wr, RFC5444_MSGTLV_INTERVAL_TIME, 0,
     	&time_encoded, sizeof(time_encoded));
#ifdef DEBUG
    rfc5444_writer_add_messagetlv(wr, RFC5444_TLV_NODE_NAME, 0, node_name, strlen(node_name));
#endif
}

static void
_cb_add_nhdp_addresses(struct rfc5444_writer *wr) {
	struct nhdp_node* neighbor;

  /* add all neighbors */
  avl_for_each_element(&nhdp_head, neighbor, node) {
		struct rfc5444_writer_address *address = rfc5444_writer_add_address(wr, _nhdp_message_content_provider.creator, neighbor->addr, false);
		rfc5444_writer_add_addrtlv(wr, address, &_nhdp_addrtlvs[IDX_ADDRTLV_LINK_STATUS], &neighbor->linkstatus, sizeof neighbor->linkstatus, false);
    if (neighbor->mpr_neigh > 0) /* node is a mpr - TODO sensible value*/
      rfc5444_writer_add_addrtlv(wr, address, &_nhdp_addrtlvs[IDX_ADDRTLV_MPR], &neighbor->mpr_neigh, sizeof neighbor->mpr_neigh, false);
#ifdef DEBUG
    if (neighbor->name)
      rfc5444_writer_add_addrtlv(wr, address, &_nhdp_addrtlvs[IDX_ADDRTLV_NODE_NAME], neighbor->name, strlen(neighbor->name), false);
#endif    
	}
}

static void
_cb_add_olsr_message_TLVs(struct rfc5444_writer *wr) {
  uint8_t time_encoded;

  time_encoded = rfc5444_timetlv_encode(REFRESH_INTERVAL);
    rfc5444_writer_add_messagetlv(wr, RFC5444_MSGTLV_VALIDITY_TIME, 0,
      &time_encoded, sizeof(time_encoded));

    time_encoded = rfc5444_timetlv_encode(HOLD_TIME);
    rfc5444_writer_add_messagetlv(wr, RFC5444_MSGTLV_INTERVAL_TIME, 0,
      &time_encoded, sizeof(time_encoded));
#ifdef DEBUG
    rfc5444_writer_add_messagetlv(wr, RFC5444_TLV_NODE_NAME, 0, node_name, strlen(node_name));
#endif
}

static void
_cb_add_olsr_addresses(struct rfc5444_writer *wr) {
  struct nhdp_node* node;

  /* add all neighbors */
  avl_for_each_element(&nhdp_head, node, node) {
    if (node->mpr_selector) {
      struct rfc5444_writer_address *address = rfc5444_writer_add_address(wr, _olsr_message_content_provider.creator, node->addr, false);
#ifdef DEBUG
      if (node->name)
        rfc5444_writer_add_addrtlv(wr, address, &_olsr_addrtlvs[IDX_ADDRTLV_NODE_NAME], node->name, strlen(node->name), false);
#endif
    }
  }
}

/**
 * Callback to define the message header for a RFC5444 message
 * @param wr
 * @param message
 */
static void
_cb_add_hello_message_header(struct rfc5444_writer *wr, struct rfc5444_writer_message *message) {
	/* originator, not hopcount, no hoplimit, sequence number */
	rfc5444_writer_set_msg_header(wr, message, true, false, false, true);
	rfc5444_writer_set_msg_originator(wr, message, netaddr_get_binptr(&local_addr));
}

static void
_cb_add_tc_message_header(struct rfc5444_writer *wr, struct rfc5444_writer_message *message) {
  /* originator, not hopcount, no hoplimit, sequence number */
  rfc5444_writer_set_msg_header(wr, message, true, false, false, true);
  rfc5444_writer_set_msg_originator(wr, message, netaddr_get_binptr(&local_addr));
}

/**
 * Initialize RFC5444 writer
 * @param ptr pointer to "send_packet" function
 */
void
writer_init(write_packet_func_ptr ptr) {
  struct rfc5444_writer_message *_hello_msg;
  struct rfc5444_writer_message *_tc_msg;

  writer.msg_buffer = msg_buffer;
  writer.msg_size   = sizeof(msg_buffer);
  writer.addrtlv_buffer = msg_addrtlvs;
  writer.addrtlv_size   = sizeof(msg_addrtlvs);

  interface.packet_buffer = packet_buffer;
  interface.packet_size   = sizeof(packet_buffer);
  interface.sendPacket    = ptr;

  /* initialize writer */
  rfc5444_writer_init(&writer);

  /* register a target (for sending messages to) in writer */
  rfc5444_writer_register_target(&writer, &interface);

  /* register a message content provider */
  rfc5444_writer_register_msgcontentprovider(&writer, &_nhdp_message_content_provider, _nhdp_addrtlvs, ARRAYSIZE(_nhdp_addrtlvs));
  rfc5444_writer_register_msgcontentprovider(&writer, &_olsr_message_content_provider, _olsr_addrtlvs, ARRAYSIZE(_olsr_addrtlvs));

  /* register message type 1 with 16 byte addresses */
  _hello_msg = rfc5444_writer_register_message(&writer, RFC5444_MSGTYPE_HELLO, false, 16);
  _tc_msg = rfc5444_writer_register_message(&writer, RFC5444_MSGTYPE_TC, false, 16);

  _hello_msg->addMessageHeader = _cb_add_hello_message_header;
  _tc_msg->addMessageHeader = _cb_add_tc_message_header;
}

void writer_send_hello(void) {
	printf("[HELLO]\n");

  print_neighbors();

	/* send message */
	rfc5444_writer_create_message_alltarget(&writer, RFC5444_MSGTYPE_HELLO);
	rfc5444_writer_flush(&writer, &interface, false);
}

void writer_send_tc(void) {
  if (!send_tc_messages)
    return;

  printf("[TC]\n");

  /* send message */
  rfc5444_writer_create_message_alltarget(&writer, RFC5444_MSGTYPE_TC);
  rfc5444_writer_flush(&writer, &interface, false);
}

/**
 * Cleanup RFC5444 writer
 */
void
writer_cleanup(void) {
  rfc5444_writer_cleanup(&writer);
}
