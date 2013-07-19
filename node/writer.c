
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

#include <string.h>
#include <stdio.h>

#include "common/common_types.h"
#include "common/netaddr.h"
#include "rfc5444/rfc5444_writer.h"

#include "writer.h"

static void _cb_addMessageTLVs(struct rfc5444_writer *wr);
static void _cb_addAddresses(struct rfc5444_writer *wr);

static struct rfc5444_writer_content_provider _message_content_provider = {
	.msg_type = 1,
	.addMessageTLVs = _cb_addMessageTLVs,
	.addAddresses = _cb_addAddresses,
};

static struct rfc5444_writer_tlvtype addrtlvs[] = {
	{ .type = 0 },
};

uint8_t msg_buffer[128];
uint8_t msg_addrtlvs[1000];
uint8_t packet_buffer[128];

struct rfc5444_writer writer;
struct rfc5444_writer_target interface;

/**
 * Callback to add message TLVs to a RFC5444 message
 * @param wr
 */
static void
_cb_addMessageTLVs(struct rfc5444_writer *wr) {
	int foo;
  printf("%s(%p)\n", __func__, wr);

	/* add message tlv type 0 (ext 0) with 4-byte value 23 */
	foo = htonl(23);
	rfc5444_writer_add_messagetlv(wr, 0, 0, &foo, sizeof (foo));

  /* add message tlv type 1 (ext 0) with 4-byte value 42 */
	foo = htonl(42);
	rfc5444_writer_add_messagetlv(wr, 1, 0, &foo, sizeof (foo));
	foo = htonl(5);
	rfc5444_writer_add_messagetlv(wr, 1, 0, &foo, sizeof (foo));
}

static void
_cb_addAddresses(struct rfc5444_writer *wr) {
  printf("%s(%p)\n", __func__, wr);

	struct netaddr ip0 = { { 127,0,0,1}, AF_INET, 32 };
	struct netaddr ip1 = { { 127,0,0,42}, AF_INET, 32 };
	int value = htonl(65535);

	/* add an address with a tlv attached */
	struct rfc5444_writer_address *addr = rfc5444_writer_add_address(wr, _message_content_provider.creator, &ip0, false);
	rfc5444_writer_add_addrtlv(wr, addr, &addrtlvs[0], &value, sizeof value, false);

	/* add an address without an tvl */
	rfc5444_writer_add_address(wr, _message_content_provider.creator, &ip1, false);
}

/**
 * Callback to define the message header for a RFC5444 message
 * @param wr
 * @param message
 */
static void
_cb_addMessageHeader(struct rfc5444_writer *wr, struct rfc5444_writer_message *message) {
  printf("%s(%p)\n", __func__, wr);

	/* no originator, no sequence number, not hopcount, no hoplimit */
	rfc5444_writer_set_msg_header(wr, message, false, false, false, false);
}

/**
 * Initialize RFC5444 writer
 * @param ptr pointer to "send_packet" function
 */
void
writer_init(write_packet_func_ptr ptr) {
  struct rfc5444_writer_message *_msg;

  printf("%s()\n", __func__);

  writer.msg_buffer = msg_buffer;
  writer.msg_size   = sizeof(msg_buffer);
  writer.addrtlv_buffer = msg_addrtlvs;
  writer.addrtlv_size   = sizeof(msg_addrtlvs);

  interface.packet_buffer = packet_buffer;
  interface.packet_size   = sizeof(packet_buffer);
  interface.sendPacket = ptr;

  /* initialize writer */
  rfc5444_writer_init(&writer);

  /* register a target (for sending messages to) in writer */
  rfc5444_writer_register_target(&writer, &interface);

  /* register a message content provider */
  rfc5444_writer_register_msgcontentprovider(&writer, &_message_content_provider, addrtlvs, ARRAYSIZE(addrtlvs));

  /* register message type 1 with 4 byte addresses */
  _msg = rfc5444_writer_register_message(&writer, 1, false, 4);
  _msg->addMessageHeader = _cb_addMessageHeader;
}

void writer_tick() {
	/* send message */
	rfc5444_writer_create_message_alltarget(&writer, 1);
	rfc5444_writer_flush(&writer, &interface, false);
}

/**
 * Cleanup RFC5444 writer
 */
void
writer_cleanup() {
  printf("%s()\n", __func__);

  rfc5444_writer_cleanup(&writer);
}
