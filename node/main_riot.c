#ifdef RIOT
#include <vtimer.h>
#include <socket.h>
#include <inet_pton.h>
#include <sixlowpan/ip.h>

#include "rfc5444/rfc5444_writer.h"

#include "constants.h"
#include "debug.h"
#include "node.h"
#include "olsr.h"
#include "reader.h"
#include "writer.h"

void enable_receive(void) {
	// TODO
}

void disable_receive(void) {
	// TODO
}

void write_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
	struct rfc5444_writer_target *iface __attribute__((unused)),
	void *buffer, size_t length) {

	ipv6_addr_t bcast;
	ipv6_addr_set_all_nodes_addr(&bcast);

	DEBUG("write_packet(%d bytes)", length);
	ipv6_sendto(&bcast, 0, buffer, length);

}

int main(int argc, char** argv) {
	const char* this_ip;

	this_ip = "2001::1";
	local_name = strdup("A");

	node_init();

	local_addr->_type = AF_INET6;
	local_addr->_prefix_len = 128;
	inet_pton(AF_INET6, this_ip, local_addr->_addr);

	DEBUG("starting node %s with IP %s",
		local_name, netaddr_to_str_s(&nbuf[0], local_addr));

	reader_init();
	writer_init(write_packet);

	while (1) {
		sleep_s(REFRESH_INTERVAL);

		disable_receive();

		remove_expired(0);

		// print_neighbors();
		print_topology_set();
		print_routing_graph();

		writer_send_hello();
		writer_send_tc();

		DEBUG_TICK;
		enable_receive();
	}

	reader_cleanup();
	writer_cleanup();
}

#endif /* RIOT */