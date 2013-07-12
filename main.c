#include <stdio.h>
#include <stdlib.h>

#include "rfc5444/rfc5444_reader.h"
#include "rfc5444/rfc5444_writer.h"

struct node {
	struct rfc5444_reader* reader;
	struct rfc5444_writer* writer;
	struct connection* connections;

	struct node* next;
};

struct connection {
	struct node* node;
	float loss;

	struct connection* next;
};

void connect_node(struct node* node_a, struct node* node_b, float loss, bool bidirectional) {
	if (bidirectional)
		connect_node(node_b, node_a, loss, false);

	printf("%p -> %p\n", node_a, node_b);

	if (node_a->connections == 0) {
		node_a->connections = malloc(sizeof (struct connection));
		node_a->connections->node = node_b;
		node_a->connections->loss = loss;
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
struct node* add_node(struct rfc5444_reader* reader, struct rfc5444_writer* writer) {
	struct node* old_head = node_head;
	node_head = malloc(sizeof(struct node));
	node_head->next = old_head;
	node_head->reader = reader;
	node_head->writer = writer;

	return node_head;
}

void write_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
	struct rfc5444_writer_target *iface __attribute__((unused)),
	void *buffer, size_t length) {

	struct node* head = node_head;
	while (head) {
		if (head->writer == wr) {
			struct connection* con = head->connections;
			while (con) {
				printf("Sending '%s' to %p\n", buffer, con->node);
				rfc5444_reader_handle_packet(con->node->reader, buffer, length);
				con = con->next;
			}
			return;
		}
		head = head->next;
	}

	printf("Error: can't write package - node not found\n");
}

int main() {

	struct node* A = add_node(0, (struct rfc5444_writer*) 1);
	struct node* B = add_node(0, (struct rfc5444_writer*) 2);
	struct node* C = add_node(0, (struct rfc5444_writer*) 3);

	connect_node(A, B, 0, true);
	connect_node(A, C, 0, true);

	char buffer[] = "Hello World!";
	write_packet((struct rfc5444_writer*) 1, 0, buffer, sizeof buffer);

	return 0;
}
