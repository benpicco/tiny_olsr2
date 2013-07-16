#ifndef NODE_DATA_
#define NODE_DATA_

#include "rfc5444/rfc5444_reader.h"
#include "rfc5444/rfc5444_writer.h"

typedef void (*write_packet_func_ptr)(
    struct rfc5444_writer *wr, struct rfc5444_writer_target *iface, void *buffer, size_t length);

struct node_data {
	uint8_t msg_buffer[128];
	uint8_t msg_addrtlvs[1000];
	uint8_t packet_buffer[128];

	struct rfc5444_reader reader;
	struct rfc5444_writer writer;
	struct rfc5444_writer_target interface;

	struct netaddr addr;

};

struct node_data* init_node_data(struct node_data* n, write_packet_func_ptr ptr);
void cleanup(struct node_data* n);
void tick(struct node_data* n);


#endif /* NODE_DATA_ */