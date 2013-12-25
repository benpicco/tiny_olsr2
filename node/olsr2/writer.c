#ifdef RIOT
#include "net_help.h"
#endif

#ifdef ENABLE_NAME
#include <string.h>
#endif

#include "common/avl.h"
#include "common/common_types.h"
#include "common/netaddr.h"
#include "rfc5444/rfc5444.h"
#include "rfc5444/rfc5444_iana.h"
#include "rfc5444/rfc5444_writer.h"

#include "constants.h"
#include "writer.h"
#include "nhdp.h"
#include "olsr.h"
#include "debug.h"

uint8_t msg_buffer[256];
uint8_t msg_addrtlvs[512];
uint8_t packet_buffer[256];

uint16_t seq_no = 1;

static void _cb_add_nhdp_message_TLVs(struct rfc5444_writer *wr);
static void _cb_add_nhdp_addresses(struct rfc5444_writer *wr);

static void _cb_add_olsr_message_TLVs(struct rfc5444_writer *wr);
static void _cb_add_olsr_addresses(struct rfc5444_writer *wr);

/* HELLO message */
static struct rfc5444_writer_content_provider _nhdp_message_content_provider = {
	.msg_type = RFC5444_MSGTYPE_HELLO,
	.addMessageTLVs = _cb_add_nhdp_message_TLVs,
	.addAddresses = _cb_add_nhdp_addresses,
};

static struct rfc5444_writer_tlvtype _nhdp_addrtlvs[] = {
	[IDX_ADDRTLV_MPR] = { .type = RFC5444_ADDRTLV_MPR },
#ifdef ENABLE_NAME
	[IDX_ADDRTLV_NODE_NAME] = { .type = RFC5444_TLV_NODE_NAME },
#endif
};

/* TC message */
static struct rfc5444_writer_content_provider _olsr_message_content_provider = {
	.msg_type = RFC5444_MSGTYPE_TC,
	.addMessageTLVs = _cb_add_olsr_message_TLVs,
	.addAddresses = _cb_add_olsr_addresses,
};

static struct rfc5444_writer_tlvtype _olsr_addrtlvs[] = {
#ifdef ENABLE_NAME
	[IDX_ADDRTLV_NODE_NAME] = { .type = RFC5444_TLV_NODE_NAME },
#endif
	[IDX_ADDRTLV_METRIC] = { .type = RFC5444_LINKMETRIC_OUTGOING_LINK },
};

/* add TLVs to HELLO message */
static void
_cb_add_nhdp_message_TLVs(struct rfc5444_writer *wr) {
	uint8_t time_encoded = rfc5444_timetlv_encode(HELLO_REFRESH_INTERVAL);
	rfc5444_writer_add_messagetlv(wr, RFC5444_MSGTLV_VALIDITY_TIME, 0, &time_encoded, sizeof(time_encoded));

#ifdef ENABLE_NAME
	rfc5444_writer_add_messagetlv(wr, RFC5444_TLV_NODE_NAME, 0, local_name, strlen(local_name) + 1);
#endif
}

/* add addresses to HELLO message */
static void
_cb_add_nhdp_addresses(struct rfc5444_writer *wr) {
	struct olsr_node* neighbor;

	/* add all neighbors */
	avl_for_each_element(get_olsr_head(), neighbor, node) {
		if (neighbor->distance != 1)
			continue;

		if (neighbor->pending)
			continue;

		struct rfc5444_writer_address *address = rfc5444_writer_add_address(wr, 
			_nhdp_message_content_provider.creator, neighbor->addr, false);

		/* node is a mpr - TODO sensible value*/
		if (h1_deriv(neighbor)->mpr_neigh > 0) 
			rfc5444_writer_add_addrtlv(wr, address, &_nhdp_addrtlvs[IDX_ADDRTLV_MPR],
				&h1_deriv(neighbor)->mpr_neigh, sizeof h1_deriv(neighbor)->mpr_neigh, false);
#ifdef ENABLE_NAME
		if (neighbor->name)
			rfc5444_writer_add_addrtlv(wr, address, &_nhdp_addrtlvs[IDX_ADDRTLV_NODE_NAME],
				neighbor->name, strlen(neighbor->name) + 1, false);
#endif
	}
}

/* add TLVs to TC message */
static void
_cb_add_olsr_message_TLVs(struct rfc5444_writer *wr) {
	uint8_t time_encoded = rfc5444_timetlv_encode(TC_REFRESH_INTERVAL);
	rfc5444_writer_add_messagetlv(wr, RFC5444_MSGTLV_VALIDITY_TIME, 0, &time_encoded, sizeof(time_encoded));

#ifdef ENABLE_NAME
	rfc5444_writer_add_messagetlv(wr, RFC5444_TLV_NODE_NAME, 0, local_name, strlen(local_name) + 1);
#endif
}

/* add addresses to TC message */
static void
_cb_add_olsr_addresses(struct rfc5444_writer *wr) {
	struct olsr_node* node;

	/* add all neighbors */
	avl_for_each_element(get_olsr_head(), node, node) {
		if (node->distance != 1)
			continue;

		if (!node->mpr_selector)
			continue;

		if (node->pending)
			continue;

		struct rfc5444_writer_address *address __attribute__((unused));
		address = rfc5444_writer_add_address(wr, _olsr_message_content_provider.creator, node->addr, false);

#ifdef ENABLE_NAME
		if (node->name)
			rfc5444_writer_add_addrtlv(wr, address, &_olsr_addrtlvs[IDX_ADDRTLV_NODE_NAME],
				node->name, strlen(node->name) + 1, false);
#endif
	}
}

/* header for HELLO messages */
static void
_cb_add_hello_message_header(struct rfc5444_writer *wr, struct rfc5444_writer_message *message) {
	/* no originator, no hopcount, no hoplimit, no sequence number */
	rfc5444_writer_set_msg_header(wr, message, false, false, false, false);
}

/* header for TC messages */
static void
_cb_add_tc_message_header(struct rfc5444_writer *wr, struct rfc5444_writer_message *message) {
	/* originator, hopcount, hoplimit, sequence number */
	rfc5444_writer_set_msg_header(wr, message, true, true, true, true);
	rfc5444_writer_set_msg_seqno(wr, message, seq_no++);
	rfc5444_writer_set_msg_originator(wr, message, netaddr_get_binptr(get_local_addr()));

	message->hoplimit = TC_HOP_LIMIT;
}

/* reader has already decided whether to forward or not, just say ok to that */
bool
olsr_message_forwarding_selector(struct rfc5444_writer_target *rfc5444_target __attribute__((unused))) {
	return true;
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
	writer.msg_size = sizeof(msg_buffer);
	writer.addrtlv_buffer = msg_addrtlvs;
	writer.addrtlv_size	 = sizeof(msg_addrtlvs);

	interface.packet_buffer = packet_buffer;
	interface.packet_size	= sizeof(packet_buffer);
	interface.sendPacket	= ptr;

	/* initialize writer */
	rfc5444_writer_init(&writer);

	/* register a target (for sending messages to) in writer */
	rfc5444_writer_register_target(&writer, &interface);

	/* register a message content provider */
	rfc5444_writer_register_msgcontentprovider(&writer, &_nhdp_message_content_provider, _nhdp_addrtlvs, ARRAYSIZE(_nhdp_addrtlvs));
	rfc5444_writer_register_msgcontentprovider(&writer, &_olsr_message_content_provider, _olsr_addrtlvs, ARRAYSIZE(_olsr_addrtlvs));

	/* register message type 1 with 16 byte addresses */
	_hello_msg = rfc5444_writer_register_message(&writer, RFC5444_MSGTYPE_HELLO, false, RFC5444_MAX_ADDRLEN);
	_tc_msg = rfc5444_writer_register_message(&writer, RFC5444_MSGTYPE_TC, false, RFC5444_MAX_ADDRLEN);

	_hello_msg->addMessageHeader = _cb_add_hello_message_header;
	_tc_msg->addMessageHeader = _cb_add_tc_message_header;
	_tc_msg->forward_target_selector = olsr_message_forwarding_selector;
}

void writer_send_hello(void) {
	DEBUG("[HELLO]");

	/* send message */
	rfc5444_writer_create_message_alltarget(&writer, RFC5444_MSGTYPE_HELLO);
	rfc5444_writer_flush(&writer, &interface, false);

	remove_expired(0);
}

void writer_send_tc(void) {
	if (!is_sending_tc())
		return;

	DEBUG("[TC]");

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
