#include <stdlib.h>
#include <stdio.h>

#include "common/common_types.h"
#include "common/netaddr.h"
#include "rfc5444/rfc5444.h"
#include "rfc5444/rfc5444_iana.h"
#include "rfc5444/rfc5444_reader.h"

#ifdef RIOT
#include "net_help.h"
#endif

#include "debug.h"
#include "nhdp.h"
#include "olsr.h"
#include "reader.h"
#include "writer.h"
#include "constants.h"
#include "routing.h"

static struct rfc5444_reader reader;
static struct netaddr* current_src;
static struct olsr_node* current_node;

/* ughh… these variables are needed in the addr callback, but read in the packet callback */
static uint8_t vtime;
static uint8_t hops;
static uint16_t _seq_no;

static enum rfc5444_result _cb_nhdp_blocktlv_packet_okay(struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_nhdp_blocktlv_address_okay(struct rfc5444_reader_tlvblock_context *cont);

static enum rfc5444_result _cb_olsr_blocktlv_packet_okay(struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_olsr_blocktlv_address_okay(struct rfc5444_reader_tlvblock_context *cont);

/* HELLO message */
static struct rfc5444_reader_tlvblock_consumer_entry _nhdp_message_tlvs[] = {
	[IDX_TLV_VTIME] = { .type = RFC5444_MSGTLV_VALIDITY_TIME, .mandatory = true },
#ifdef ENABLE_NAME
	[IDX_TLV_NODE_NAME] = { .type = RFC5444_TLV_NODE_NAME },
#endif
};

static struct rfc5444_reader_tlvblock_consumer_entry _nhdp_address_tlvs[] = {
	[IDX_ADDRTLV_MPR] = { .type = RFC5444_ADDRTLV_MPR },
	[IDX_ADDRTLV_LINK_STATUS] = { .type = RFC5444_ADDRTLV_LINK_STATUS },
#ifdef ENABLE_NAME
	[IDX_ADDRTLV_NODE_NAME] = { .type = RFC5444_TLV_NODE_NAME },
#endif
};

/* TC message */
static struct rfc5444_reader_tlvblock_consumer_entry _olsr_message_tlvs[] = {
	[IDX_TLV_VTIME] = { .type = RFC5444_MSGTLV_VALIDITY_TIME, .mandatory = true },
#ifdef ENABLE_NAME
	[IDX_TLV_NODE_NAME] = { .type = RFC5444_TLV_NODE_NAME },
#endif
};

static struct rfc5444_reader_tlvblock_consumer_entry _olsr_address_tlvs[] = {
	[IDX_ADDRTLV_LINK_STATUS] = { .type = RFC5444_ADDRTLV_LINK_STATUS },
#ifdef ENABLE_NAME
	[IDX_ADDRTLV_NODE_NAME] = { .type = RFC5444_TLV_NODE_NAME },
#endif
};

/* define callbacks for HELLO message */
static struct rfc5444_reader_tlvblock_consumer _nhdp_consumer = {
	.msg_id = RFC5444_MSGTYPE_HELLO,
	.block_callback = _cb_nhdp_blocktlv_packet_okay,
};

static struct rfc5444_reader_tlvblock_consumer _nhdp_address_consumer = {
	.msg_id = RFC5444_MSGTYPE_HELLO,
	.addrblock_consumer = true,
	.block_callback = _cb_nhdp_blocktlv_address_okay,
};

/* define callbacks for TC message */
static struct rfc5444_reader_tlvblock_consumer _olsr_consumer = {
	.msg_id = RFC5444_MSGTYPE_TC,
	.block_callback = _cb_olsr_blocktlv_packet_okay,
};

static struct rfc5444_reader_tlvblock_consumer _olsr_address_consumer = {
	.msg_id = RFC5444_MSGTYPE_TC,
	.addrblock_consumer = true,
	.block_callback = _cb_olsr_blocktlv_address_okay,
};

/* HELLO message */
static enum rfc5444_result
_cb_nhdp_blocktlv_packet_okay(struct rfc5444_reader_tlvblock_context *cont __attribute__((unused))) {
	DEBUG("received HELLO message:");

	if (netaddr_cmp(get_local_addr(), current_src) == 0)
		return RFC5444_DROP_PACKET;

	/* VTIME is defined as mandatory */
	vtime = rfc5444_timetlv_decode(*_nhdp_message_tlvs[IDX_TLV_VTIME].tlv->single_value);

	char* name = NULL;
#ifdef ENABLE_NAME
	if (_nhdp_message_tlvs[IDX_TLV_NODE_NAME].tlv) {
		name = (char*) _nhdp_message_tlvs[IDX_TLV_NODE_NAME].tlv->single_value;
		DEBUG("\tfrom: %s (%s)", name, netaddr_to_str_s(&nbuf[0], current_src));
	}
#endif

	current_node = add_neighbor(current_src, vtime, name);

	if (current_node == NULL) {
		puts("ERROR: add_neighbor failed - out of memory");
		return RFC5444_DROP_PACKET;
	}

	/* reset MPR selector state, will be set by _cb_nhdp_blocktlv_address_okay */
	current_node->mpr_selector = 0;

	if (current_node->pending)
		return RFC5444_DROP_PACKET;

	return RFC5444_OKAY;
}

/* HELLO announced addresses */
static enum rfc5444_result
_cb_nhdp_blocktlv_address_okay(struct rfc5444_reader_tlvblock_context *cont) {
	struct rfc5444_reader_tlvblock_entry* tlv;

	char* name = NULL;
#ifdef ENABLE_NAME
	if ((tlv = _nhdp_address_tlvs[IDX_ADDRTLV_NODE_NAME].tlv)) {
		name = (char*) tlv->single_value;
		DEBUG("\t2-hop neighbor: %s (%s)", name, netaddr_to_str_s(&nbuf[0], &cont->addr));
	}
#endif

	if ((tlv = _nhdp_address_tlvs[IDX_ADDRTLV_LINK_STATUS].tlv)) {
		switch (* (char*) tlv->single_value) {
		struct olsr_node* lost;
		case RFC5444_LINKSTATUS_LOST:
			lost = get_node(&cont->addr);
			DEBUG("\texpired node reported, removing it (HELLO)%s", lost ? "" : " [not found]");

			if (lost != NULL)
				route_expired(lost, current_node->addr);

			return RFC5444_DROP_ADDRESS;
		default:
			DEBUG("\tunknown LINKSTATUS = %d", * (char*) tlv->single_value);
		}
	}

	/* node broadcasts us as it's neighbor */
	if (netaddr_cmp(&cont->addr, get_local_addr()) == 0) {

		/* node selected us as mpr */
		if ((tlv = _nhdp_address_tlvs[IDX_ADDRTLV_MPR].tlv))
			current_node->mpr_selector = 1;

	} else
		add_olsr_node(&cont->addr, current_src, vtime, 2, name);

	return RFC5444_OKAY;
}

/* TC message */
static enum rfc5444_result
_cb_olsr_blocktlv_packet_okay(struct rfc5444_reader_tlvblock_context *cont) {
	DEBUG("received TC message:");

	if (!cont->has_origaddr)
		return RFC5444_DROP_PACKET;

	if (!cont->has_seqno)
		return RFC5444_DROP_PACKET;

	if (!cont->has_hopcount || !cont->has_hoplimit)
		return RFC5444_DROP_PACKET;

	if (!netaddr_cmp(get_local_addr(), current_src))
		return RFC5444_DROP_PACKET;

	if (!netaddr_cmp(get_local_addr(), &cont->orig_addr))
		return RFC5444_DROP_PACKET;

	vtime = rfc5444_timetlv_decode(*_olsr_message_tlvs[IDX_TLV_VTIME].tlv->single_value);

	if (is_known_msg(&cont->orig_addr, cont->seqno, vtime))
		return RFC5444_DROP_PACKET;

	DEBUG("\tfrom: %s", netaddr_to_str_s(&nbuf[0], &cont->orig_addr));
	DEBUG("\tsender: %s", netaddr_to_str_s(&nbuf[0], current_src));
	DEBUG("\tseqno: %d", cont->seqno);
	DEBUG("\thops: %d", cont->hopcount);

	hops = cont->hopcount + 1; /* hopcount starts with 0 for A -> B */
	_seq_no = cont->seqno;

	return RFC5444_OKAY;
}

/* TC announced addresses */
static enum rfc5444_result
_cb_olsr_blocktlv_address_okay(struct rfc5444_reader_tlvblock_context *cont) {
	struct rfc5444_reader_tlvblock_entry* tlv __attribute__((unused));
	char* name = NULL;

	if (netaddr_cmp(get_local_addr(), &cont->addr) == 0)
		return RFC5444_DROP_ADDRESS;

#ifdef ENABLE_NAME
	if ((tlv = _olsr_address_tlvs[IDX_ADDRTLV_NODE_NAME].tlv)) {
		name = (char*) tlv->single_value;
		DEBUG("\tannounces: %s (%s)", name, netaddr_to_str_s(&nbuf[0], &cont->addr));
	}
#endif

	if ((tlv = _olsr_address_tlvs[IDX_ADDRTLV_LINK_STATUS].tlv)) {
		switch (* (char*) tlv->single_value) {
		struct olsr_node* lost;
		case RFC5444_LINKSTATUS_LOST:
			lost = get_node(&cont->addr);
			DEBUG("\texpired node reported, removing it (TC)%s", lost ? "" : " [not found]");

			if (lost != NULL)
				route_expired(lost, current_node->addr);

			return RFC5444_DROP_ADDRESS;
		default:
			DEBUG("\tunknown LINKSTATUS = %d", * (char*) tlv->single_value);
		}
	}

	/* hops is hopcount to orig_addr, addr is one more hop */
	add_olsr_node(&cont->addr, &cont->orig_addr, vtime, hops + 1, name);

	return RFC5444_OKAY;
}

/* this is only called for messages with hopcount/hoplimit, that is only TC messages */
static void
_cb_olsr_forward_message(struct rfc5444_reader_tlvblock_context *context __attribute__((unused)),
	uint8_t *buffer, size_t length) {
	struct olsr_node* node = get_node(current_src);

	/* only forward if node selected us as MPR */
	if (node == NULL || node->mpr_selector == 0)
		return;

	if (RFC5444_OKAY == rfc5444_writer_forward_msg(&writer, buffer, length))
		rfc5444_writer_flush(&writer, &interface, true);
	else
		DEBUG("\tfailed forwarding package");
}

/**
 * Initialize RFC5444 reader
 */
void reader_init(void) {
	/* initialize reader */
	rfc5444_reader_init(&reader);
	reader.forward_message = _cb_olsr_forward_message;

	/* register HELLO message consumer */
	rfc5444_reader_add_message_consumer(&reader, &_nhdp_consumer, _nhdp_message_tlvs, ARRAYSIZE(_nhdp_message_tlvs));
	rfc5444_reader_add_message_consumer(&reader, &_nhdp_address_consumer, _nhdp_address_tlvs, ARRAYSIZE(_nhdp_address_tlvs));

	/* register TC message consumer */
	rfc5444_reader_add_message_consumer(&reader, &_olsr_consumer, _olsr_message_tlvs, ARRAYSIZE(_olsr_message_tlvs));
	rfc5444_reader_add_message_consumer(&reader, &_olsr_address_consumer, _olsr_address_tlvs, ARRAYSIZE(_olsr_address_tlvs));
}

/**
 * Inject a package into the RFC5444 reader
 */
int reader_handle_packet(void* buffer, size_t length, struct netaddr* src) {
	current_src = src;
	return rfc5444_reader_handle_packet(&reader, buffer, length);
}

/**
 * Cleanup RFC5444 reader
 */
void reader_cleanup(void) {
	rfc5444_reader_cleanup(&reader);
}
