/*
 * The olsr.org Optimized Link-State Routing daemon version 2 (olsrd2)
 * Copyright (c) 2004-2013, the olsr.org team - see HISTORY file
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * * Neither the name of olsr.org, olsrd nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Visit http://www.olsr.org for more information.
 *
 * If you find this software useful feel free to make a donation
 * to the project. For more information see the website or contact
 * the copyright holders.
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include "common/common_types.h"
#include "common/netaddr.h"
#include "rfc5444/rfc5444.h"
#include "rfc5444/rfc5444_iana.h"
#include "rfc5444/rfc5444_reader.h"

#ifdef RIOT
#include "net_help/net_help.h"
#endif

#ifdef DEBUG
#include <string.h>
#endif

#include "nhdp.h"
#include "nhdp_reader.h"
#include "olsr2.h"
#include "constants.h"

static enum rfc5444_result _cb_blocktlv_packet_okay(
    struct rfc5444_reader_tlvblock_context *cont);

static enum rfc5444_result _cb_blocktlv_address_okay(
    struct rfc5444_reader_tlvblock_context *cont);

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

static struct rfc5444_reader_tlvblock_consumer _consumer = {
  .msg_id = RFC5444_MSGTYPE_HELLO,
  .block_callback = _cb_blocktlv_packet_okay,
};

static struct rfc5444_reader_tlvblock_consumer _address_consumer = {
  .msg_id = RFC5444_MSGTYPE_HELLO,
  .addrblock_consumer = true,
  .block_callback = _cb_blocktlv_address_okay,
};

struct rfc5444_reader reader;
struct nhdp_node* current_node;

/**
 * This block callback is only called if message tlv type 1 is present,
 * because it was declared as mandatory
 *
 * @param cont
 * @return
 */
static enum rfc5444_result
_cb_blocktlv_packet_okay(struct rfc5444_reader_tlvblock_context *cont) {
  uint8_t value;
  struct netaddr_str nbuf;

  printf("received package:\n");

  if (cont->has_origaddr) {
    printf("\torig_addr: %s\n", netaddr_to_string(&nbuf, &cont->orig_addr));
    current_node = add_neighbor(&cont->orig_addr, RFC5444_LINKSTATUS_HEARD);
  }

  if (cont->has_seqno) {
    printf("\tseqno: %d\n", cont->seqno);
  }

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
_cb_blocktlv_address_okay(struct rfc5444_reader_tlvblock_context *cont) {
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

  /* node selected us as mpr */
  if ((tlv = _nhdp_address_tlvs[IDX_ADDRTLV_MPR].tlv) && netaddr_cmp(&cont->addr, &local_addr) == 0) {
    add_routing_mpr_selector(get_neighbor(&cont->addr));
#ifdef DEBUG
    // allow MPR selection to be drawn in graphviz
    printf("\t%s -> %s // [ label=\"MPR\" ];\n", current_node->name, node_name);
#endif
  }

  add_2_hop_neighbor(current_node, &cont->addr, linkstatus, name);

  return RFC5444_OKAY;
}

/**
 * Initialize RFC5444 reader
 */
void
nhdp_reader_init(void) {
  /* initialize reader */
  rfc5444_reader_init(&reader);

  /* register message consumer */
  rfc5444_reader_add_message_consumer(&reader, &_consumer,
      _nhdp_message_tlvs, ARRAYSIZE(_nhdp_message_tlvs));

  rfc5444_reader_add_message_consumer(&reader, &_address_consumer,
      _nhdp_address_tlvs, ARRAYSIZE(_nhdp_address_tlvs));
}

/**
 * Inject a package into the RFC5444 reader
 */
int
nhdp_reader_handle_packet(void* buffer, size_t length) {
  return rfc5444_reader_handle_packet(&reader, buffer, length);
}

/**
 * Cleanup RFC5444 reader
 */
void
nhdp_reader_cleanup(void) {
  rfc5444_reader_cleanup(&reader);
}
