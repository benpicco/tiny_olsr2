#include "node.h"
#include "writer.h"
#include "reader.h"

#include "rfc5444/rfc5444_reader.h"
#include "rfc5444/rfc5444_writer.h"

struct node_data* init_node_data(struct node_data* n, write_packet_func_ptr ptr) {
	n->writer.msg_buffer = n->msg_buffer;
	n->writer.msg_size   = sizeof(n->msg_buffer);
	n->writer.addrtlv_buffer = n->msg_addrtlvs;
	n->writer.addrtlv_size   = sizeof(n->msg_addrtlvs);

	n->interface.packet_buffer = n->packet_buffer;
	n->interface.packet_size   = sizeof(n->packet_buffer);
	n->interface.sendPacket = ptr;

	writer_init(n);
	reader_init(n);

	return n;
}

void tick(struct node_data* n) {
	/* send message */
	rfc5444_writer_create_message_alltarget(&n->writer, 1);
	rfc5444_writer_flush(&n->writer, &n->interface, false);
}

void cleanup(struct node_data* n) {
	reader_cleanup(n);
	writer_cleanup(n);
}