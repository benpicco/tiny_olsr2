#include <stdlib.h>
#include <stdio.h>
#ifdef DEBUG
#include <string.h>
#endif

#include "common/common_types.h"
#include "common/netaddr.h"
#include "rfc5444/rfc5444.h"
#include "rfc5444/rfc5444_iana.h"
#include "rfc5444/rfc5444_reader.h"

#ifdef RIOT
#include "net_help/net_help.h"
#endif

#include "nhdp.h"
#include "olsr.h"
#include "reader.h"
#include "writer.h"
#include "constants.h"

struct rfc5444_reader reader;
struct netaddr* current_src;
struct nhdp_node* current_node;

/* ughh… these variables are needed in the addr callback, but read in the packet callback */
uint8_t vtime;
uint8_t hops;
uint16_t seq_no;

static enum rfc5444_result _cb_nhdp_blocktlv_packet_okay(struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_nhdp_blocktlv_address_okay(struct rfc5444_reader_tlvblock_context *cont);

static enum rfc5444_result _cb_olsr_blocktlv_packet_okay(struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_olsr_blocktlv_address_okay(struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_olsr_end_callback(struct rfc5444_reader_tlvblock_context *context, bool dropped);

/* HELLO message */
static struct rfc5444_reader_tlvblock_consumer_entry _nhdp_message_tlvs[] = {
	[IDX_TLV_ITIME] = { .type = RFC5444_MSGTLV_INTERVAL_TIME, .type_ext = 0, .match_type_ext = true,
		.mandatory = true, .min_length = 1, .match_length = true },
	[IDX_TLV_VTIME] = { .type = RFC5444_MSGTLV_VALIDITY_TIME, .type_ext = 0, .match_type_ext = true,
		.mandatory = true, .min_length = 1, .match_length = true },
	[IDX_TLV_WILLINGNESS] = { .type = RFC5444_MSGTLV_MPR_WILLING, .type_ext = 0, .match_type_ext = true,
		.min_length = 1, .match_length = true },
#ifdef DEBUG
	[IDX_TLV_NODE_NAME] = { .type = RFC5444_TLV_NODE_NAME },
#endif
};

static struct rfc5444_reader_tlvblock_consumer_entry _nhdp_address_tlvs[] = {
	[IDX_ADDRTLV_LOCAL_IF] = { .type = RFC5444_ADDRTLV_LOCAL_IF, .type_ext = 0, .match_type_ext = true,
		.min_length = 1, .match_length = true },
	[IDX_ADDRTLV_LINK_STATUS] = { .type = RFC5444_ADDRTLV_LINK_STATUS, .type_ext = 0, .match_type_ext = true,
		.min_length = 1, .match_length = true },
	[IDX_ADDRTLV_MPR] = { .type = RFC5444_ADDRTLV_MPR,
		.min_length = 1, .match_length = true },
	[IDX_ADDRTLV_LINKMETRIC] = { .type = RFC5444_ADDRTLV_LINK_METRIC, .min_length = 2, .match_length = true },
#ifdef DEBUG
	[IDX_ADDRTLV_NODE_NAME] = { .type = RFC5444_TLV_NODE_NAME },
#endif
};

/* TC message */
static struct rfc5444_reader_tlvblock_consumer_entry _olsr_message_tlvs[] = {
	[IDX_TLV_ITIME] = { .type = RFC5444_MSGTLV_INTERVAL_TIME, .type_ext = 0, .match_type_ext = true,
		.min_length = 1, .match_length = true },
	[IDX_TLV_VTIME] = { .type = RFC5444_MSGTLV_VALIDITY_TIME, .type_ext = 0, .match_type_ext = true,
		.mandatory = true, .min_length = 1, .match_length = true },
	[IDX_TLV_WILLINGNESS] = { .type = RFC5444_MSGTLV_MPR_WILLING, .type_ext = 0, .match_type_ext = true,
		.min_length = 1, .match_length = true },
#ifdef DEBUG
	[IDX_TLV_NODE_NAME] = { .type = RFC5444_TLV_NODE_NAME },
#endif
};

static struct rfc5444_reader_tlvblock_consumer_entry _olsr_address_tlvs[] = {
#ifdef DEBUG
	[IDX_ADDRTLV_NODE_NAME] = { .type = RFC5444_TLV_NODE_NAME },
#endif
};

static struct rfc5444_reader_tlvblock_consumer _nhdp_consumer = {
	.msg_id = RFC5444_MSGTYPE_HELLO,
	.block_callback = _cb_nhdp_blocktlv_packet_okay,
};

static struct rfc5444_reader_tlvblock_consumer _nhdp_address_consumer = {
	.msg_id = RFC5444_MSGTYPE_HELLO,
	.addrblock_consumer = true,
	.block_callback = _cb_nhdp_blocktlv_address_okay,
};

static struct rfc5444_reader_tlvblock_consumer _olsr_consumer = {
	.msg_id = RFC5444_MSGTYPE_TC,
	.block_callback = _cb_olsr_blocktlv_packet_okay,
	.end_callback = _cb_olsr_end_callback,
};

static struct rfc5444_reader_tlvblock_consumer _olsr_address_consumer = {
	.msg_id = RFC5444_MSGTYPE_TC,
	.addrblock_consumer = true,
	.block_callback = _cb_olsr_blocktlv_address_okay,
};

/* HELLO message */

static enum rfc5444_result
_cb_nhdp_blocktlv_packet_okay(struct rfc5444_reader_tlvblock_context *cont) {
	uint8_t value;
	struct netaddr_str nbuf;

	printf("received HELLO package:\n");

	printf("\tfrom: %s\n", netaddr_to_string(&nbuf, current_src));
	current_node = add_neighbor(current_src, RFC5444_LINKSTATUS_HEARD);

	/* both VTIME and ITIME were defined as mandatory */
	value = rfc5444_timetlv_decode(*_nhdp_message_tlvs[IDX_TLV_ITIME].tlv->single_value);
	printf("\tITIME: %d\n", value);

	value = rfc5444_timetlv_decode(*_nhdp_message_tlvs[IDX_TLV_VTIME].tlv->single_value);
	printf("\tVTIME: %d\n", value);

#ifdef DEBUG
	if (_nhdp_message_tlvs[IDX_TLV_NODE_NAME].tlv) {
		if (!current_node->name)
			current_node->name = strndup((char*) _nhdp_message_tlvs[IDX_TLV_NODE_NAME].tlv->_value, _nhdp_message_tlvs[IDX_TLV_NODE_NAME].tlv->length);
		printf("\tname: %s\n", current_node->name);
	}
#endif

	return RFC5444_OKAY;
}

static enum rfc5444_result
_cb_nhdp_blocktlv_address_okay(struct rfc5444_reader_tlvblock_context *cont) {
	struct rfc5444_reader_tlvblock_entry* tlv;
	uint8_t linkstatus = RFC5444_LINKSTATUS_HEARD;

	char* name = 0;
#ifdef DEBUG
	struct netaddr_str nbuf;
	if ((tlv = _nhdp_address_tlvs[IDX_ADDRTLV_NODE_NAME].tlv)) {
		name = strndup((char*) tlv->single_value, tlv->length); // memory leak
		printf("\t2-hop neighbor: %s (%s)\n", name, netaddr_to_string(&nbuf, &cont->addr));
	}
#endif

	if ((tlv = _nhdp_address_tlvs[IDX_ADDRTLV_LINK_STATUS].tlv))
		linkstatus = *tlv->single_value;

	/* node broadcasts us as it's neighbor */
	if (netaddr_cmp(&cont->addr, &local_addr) == 0) {
		current_node->linkstatus = RFC5444_LINKSTATUS_SYMMETRIC;

		/* node selected us as mpr */
		if ((tlv = _nhdp_address_tlvs[IDX_ADDRTLV_MPR].tlv)) {
			current_node->mpr_selector = ROUTING_MPR_SELECTOR; // arbitrary, todo
			send_tc_messages = true;
#ifdef DEBUG
			// allow MPR selection to be drawn in graphviz
			printf("\t%s -> %s // [ label=\"MPR\" ];\n", current_node->name, node_name);
#endif
		}
	} else {
	 /* no need to try adding us as a 2-hop neighbor */
		add_2_hop_neighbor(current_node, &cont->addr, linkstatus, name);
	}

	return RFC5444_OKAY;
}

/* TC message */

static enum rfc5444_result
_cb_olsr_blocktlv_packet_okay(struct rfc5444_reader_tlvblock_context *cont) {
	struct netaddr_str nbuf;

	printf("received TC package:\n");

	if (!cont->has_origaddr)
		return RFC5444_DROP_PACKET;

	if (!cont->has_seqno)
		return RFC5444_DROP_PACKET;

	if (!cont->has_hopcount || !cont->has_hoplimit)
		return RFC5444_DROP_PACKET;

  vtime = rfc5444_timetlv_decode(*_olsr_message_tlvs[IDX_TLV_VTIME].tlv->single_value);

	printf("\torig_addr: %s\n", netaddr_to_string(&nbuf, &cont->orig_addr));
	printf("\tseqno: %d\n", cont->seqno);
	printf("\tVTIME: %d\n", vtime);

  if (!netaddr_cmp(&local_addr, &cont->orig_addr))
    return RFC5444_DROP_PACKET;

  if (is_known_msg(&cont->orig_addr, cont->seqno)) {
    printf("message already processed, dropping it\n");
    return RFC5444_DROP_PACKET;
  }

  hops = cont->hopcount;
  seq_no = cont->seqno;

  // what if address block is empty?

#ifdef DEBUG
	if (_olsr_message_tlvs[IDX_TLV_NODE_NAME].tlv) {
		char* _name = strndup((char*) _olsr_message_tlvs[IDX_TLV_NODE_NAME].tlv->_value, _olsr_message_tlvs[IDX_TLV_NODE_NAME].tlv->length);
		printf("\tname: %s\n", _name);
		free(_name);
	}
#endif

	return RFC5444_OKAY;
}

static enum rfc5444_result
_cb_olsr_blocktlv_address_okay(struct rfc5444_reader_tlvblock_context *cont) {
	struct rfc5444_reader_tlvblock_entry* tlv;

#ifdef DEBUG
	struct netaddr_str nbuf;
	if ((tlv = _olsr_address_tlvs[IDX_ADDRTLV_NODE_NAME].tlv)) {
		char* name = strndup((char*) tlv->single_value, tlv->length);
		printf("\tannonces: %s (%s)\n", name, netaddr_to_string(&nbuf, &cont->addr));
		add_olsr_node(&cont->addr, current_src, seq_no, vtime, hops + 1, name);
	}
#endif

	return RFC5444_OKAY;
}

static void _cb_olsr_forward_message(struct rfc5444_reader_tlvblock_context *context, uint8_t *buffer, size_t length) {
	struct netaddr_str nbuf;
	printf("_cb_olsr_forward_message(%zd bytes)\n", length);
 
	struct nhdp_node* node = get_neighbor(current_src);
#ifdef DEBUG	
	printf("sender: %s (%s)\n", netaddr_to_string(&nbuf, current_src), node ? node->name : "null");
#endif

	if (!node) 
		printf("I don't know the sender, dropping packet.\n");

	if (node && node->mpr_selector) {
		printf("forwarding package\n");
		if (RFC5444_OKAY == rfc5444_writer_forward_msg(&writer, buffer, length))
			rfc5444_writer_flush(&writer, &interface, true);
		else
			printf("failed forwarding package\n");
	}
}

static enum rfc5444_result
_cb_olsr_end_callback(struct rfc5444_reader_tlvblock_context *context, bool dropped) {
	printf("_cb_olsr_end_callback(%s), current_src is used %d times\n", dropped ? "dropped" : "", current_src->_refs);
	netaddr_free(current_src);

	return dropped ? RFC5444_OKAY : RFC5444_DROP_PACKET;
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
	current_src = netaddr_dup(src);
	return rfc5444_reader_handle_packet(&reader, buffer, length);
}

/**
 * Cleanup RFC5444 reader
 */
void reader_cleanup(void) {
	rfc5444_reader_cleanup(&reader);
}
