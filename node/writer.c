#ifdef ENABLE_DEBUG
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

#include "constants.h"
#include "writer.h"
#include "nhdp.h"
#include "debug.h"

uint8_t msg_buffer[128];
uint8_t msg_addrtlvs[1000];
uint8_t packet_buffer[128];

uint16_t seq_no = 1;

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
	[IDX_ADDRTLV_MPR] = { .type = RFC5444_ADDRTLV_MPR },
	[IDX_ADDRTLV_LINKMETRIC] = { .type = RFC5444_ADDRTLV_LINK_METRIC },
#ifdef ENABLE_DEBUG
	[IDX_ADDRTLV_NODE_NAME] = { .type = RFC5444_TLV_NODE_NAME },
#endif
};

static struct rfc5444_writer_content_provider _olsr_message_content_provider = {
	.msg_type = RFC5444_MSGTYPE_TC,
	.addMessageTLVs = _cb_add_olsr_message_TLVs,
	.addAddresses = _cb_add_olsr_addresses,
};

static struct rfc5444_writer_tlvtype _olsr_addrtlvs[] = {
	[IDX_ADDRTLV_LINKMETRIC] = { .type = RFC5444_ADDRTLV_LINK_METRIC },
#ifdef ENABLE_DEBUG
	[IDX_ADDRTLV_NODE_NAME] = { .type = RFC5444_TLV_NODE_NAME },
#endif
};

/**
 * Callback to add message TLVs to a RFC5444 message
 * @param wr
 */
static void
_cb_add_nhdp_message_TLVs(struct rfc5444_writer *wr) {
	uint8_t time_encoded = rfc5444_timetlv_encode(REFRESH_INTERVAL);
	rfc5444_writer_add_messagetlv(wr, RFC5444_MSGTLV_VALIDITY_TIME, 0, &time_encoded, sizeof(time_encoded));

#ifdef ENABLE_DEBUG
	rfc5444_writer_add_messagetlv(wr, RFC5444_TLV_NODE_NAME, 0, local_name, strlen(local_name));
#endif
}

static void
_cb_add_nhdp_addresses(struct rfc5444_writer *wr) {
	struct olsr_node* neighbor;

	/* add all neighbors */
	avl_for_each_element(&olsr_head, neighbor, node) {
		if (neighbor->distance != 1)
			continue;

		if (h1_deriv(neighbor)->pending)
			continue;

		struct rfc5444_writer_address *address = rfc5444_writer_add_address(wr, 
			_nhdp_message_content_provider.creator, neighbor->addr, false);

		/* add link metric */
		uint8_t metric = 1 + (1 - h1_deriv(neighbor)->link_quality) * LINK_METRIC_MAX;
		rfc5444_writer_add_addrtlv(wr, address, &_nhdp_addrtlvs[IDX_ADDRTLV_LINKMETRIC], &metric, sizeof metric, false);

		/* node is a mpr - TODO sensible value*/
		if (h1_deriv(neighbor)->mpr_neigh > 0) 
			rfc5444_writer_add_addrtlv(wr, address, &_nhdp_addrtlvs[IDX_ADDRTLV_MPR],
				&h1_deriv(neighbor)->mpr_neigh, sizeof h1_deriv(neighbor)->mpr_neigh, false);
#ifdef ENABLE_DEBUG
		if (neighbor->name)
			rfc5444_writer_add_addrtlv(wr, address, &_nhdp_addrtlvs[IDX_ADDRTLV_NODE_NAME],
				neighbor->name, strlen(neighbor->name), false);
#endif
	}
}

static void
_cb_add_olsr_message_TLVs(struct rfc5444_writer *wr) {
	uint8_t time_encoded = rfc5444_timetlv_encode(REFRESH_INTERVAL);
	rfc5444_writer_add_messagetlv(wr, RFC5444_MSGTLV_VALIDITY_TIME, 0, &time_encoded, sizeof(time_encoded));

#ifdef ENABLE_DEBUG
	rfc5444_writer_add_messagetlv(wr, RFC5444_TLV_NODE_NAME, 0, local_name, strlen(local_name));
#endif
}

static void
_cb_add_olsr_addresses(struct rfc5444_writer *wr) {
	struct olsr_node* node;

	/* add all neighbors */
	avl_for_each_element(&olsr_head, node, node) {
		if (node->distance != 1)
			continue;

		if (!h1_deriv(node)->mpr_selector)
			continue;

		if (h1_deriv(node)->pending)
			continue;

		struct rfc5444_writer_address *address = rfc5444_writer_add_address(wr,
			_olsr_message_content_provider.creator, node->addr, false);

		rfc5444_writer_add_addrtlv(wr, address, &_olsr_addrtlvs[IDX_ADDRTLV_LINKMETRIC], 
			&node->link_metric, sizeof node->link_metric, false);

#ifdef ENABLE_DEBUG
		if (node->name)
			rfc5444_writer_add_addrtlv(wr, address, &_olsr_addrtlvs[IDX_ADDRTLV_NODE_NAME],
				node->name, strlen(node->name), false);
#endif
	}
}

static void
_cb_add_hello_message_header(struct rfc5444_writer *wr, struct rfc5444_writer_message *message) {
	/* no originator, no hopcount, no hoplimit, no sequence number */
	rfc5444_writer_set_msg_header(wr, message, false, false, false, false);
}

static void
_cb_add_tc_message_header(struct rfc5444_writer *wr, struct rfc5444_writer_message *message) {
	/* originator, hopcount, hoplimit, sequence number */
	rfc5444_writer_set_msg_header(wr, message, true, true, true, true);
	rfc5444_writer_set_msg_seqno(wr, message, seq_no++);
	rfc5444_writer_set_msg_originator(wr, message, netaddr_get_binptr(local_addr));

	message->hoplimit = 16;
}

bool
olsr_message_forwarding_selector(struct rfc5444_writer_target *rfc5444_target) {
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
	writer.msg_size	 = sizeof(msg_buffer);
	writer.addrtlv_buffer = msg_addrtlvs;
	writer.addrtlv_size	 = sizeof(msg_addrtlvs);

	interface.packet_buffer = packet_buffer;
	interface.packet_size	 = sizeof(packet_buffer);
	interface.sendPacket		= ptr;

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
	_tc_msg->forward_target_selector = olsr_message_forwarding_selector;
}

void writer_send_hello(void) {
	DEBUG("[HELLO]");

	/* send message */
	rfc5444_writer_create_message_alltarget(&writer, RFC5444_MSGTYPE_HELLO);
	rfc5444_writer_flush(&writer, &interface, false);
}

void writer_send_tc(void) {
	if (!send_tc_messages)
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
