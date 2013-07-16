#include <stdio.h>
#include <stdlib.h>

#include "common/common_types.h"
#include "common/netaddr.h"

#include "rfc5444/rfc5444_reader.h"
#include "rfc5444/rfc5444_writer.h"
#include "rfc5444/rfc5444_print.h"

#include "node/writer.h"
#include "node/reader.h"
#include "node/node.h"

/* for hexfump */
static struct autobuf _hexbuf;

struct node {
	struct node_data* data;
	struct connection* connections;
	struct node* next;
};

struct connection {
	struct node* node;
	struct connection* next;
};

void connect_node(struct node* node_a, struct node* node_b, bool bidirectional) {
	if (bidirectional)
		connect_node(node_b, node_a, false);

	printf("%p -> %p\n", node_a, node_b);

	if (node_a->connections == 0) {
		node_a->connections = malloc(sizeof (struct connection));
		node_a->connections->node = node_b;
		node_a->connections->next = 0;
	} else {
		struct connection* con = node_a->connections;
		while (con->next)
			con = con->next;
		con->next = malloc(sizeof (struct connection));
		con->next->node = node_b;
		con->next->next = 0;
	}
}

static struct node* node_head = 0;
struct node* add_node(struct node_data* data) {
	struct node* old_head = node_head;
	node_head = malloc(sizeof(struct node));
	node_head->next = old_head;
	node_head->data = data;

	return node_head;
}

void write_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
	struct rfc5444_writer_target *iface __attribute__((unused)),
	void *buffer, size_t length) {

	/* generate hexdump of packet */
	abuf_hexdump(&_hexbuf, "\t", buffer, length);
	rfc5444_print_direct(&_hexbuf, buffer, length);

	/* print hexdump to console */
	printf("%s", abuf_getptr(&_hexbuf));

	struct node* head = node_head;
	while (head) {
		if (&head->data->writer == wr) {
			struct connection* con = head->connections;
			while (con) {
				rfc5444_reader_handle_packet(&con->node->data->reader, buffer, length);
				con = con->next;
			}
			return;
		}
		head = head->next;
	}

	printf("Error: can't write package - node not found\n");
}

int main() {
	/* initialize buffer for hexdump */
	abuf_init(&_hexbuf);

	struct node* A = add_node(init_node_data(malloc(sizeof(struct node_data)), write_packet));
	struct node* B = add_node(init_node_data(malloc(sizeof(struct node_data)), write_packet));
	struct node* C = add_node(init_node_data(malloc(sizeof(struct node_data)), write_packet));

	connect_node(A, B, true);
	connect_node(A, C, true);

	tick(A->data);

	abuf_free(&_hexbuf);
	return 0;
}
