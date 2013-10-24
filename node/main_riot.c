#ifdef RIOT
#include <thread.h>
#include <vtimer.h>
#include <socket.h>
#include <inet_pton.h>
#include <sixlowpan/ip.h>
#include <msg.h>

#include "rfc5444/rfc5444_writer.h"

#include "constants.h"
#include "debug.h"
#include "node.h"
#include "olsr.h"
#include "reader.h"
#include "writer.h"

#if BOARD == native
static uint8_t _trans_type = TRANSCEIVER_NATIVE;
#elif BOARD == msba2
static uint8_t _trans_type = TRANSCEIVER_CC1100;
#endif

char receive_thread_stack[KERNEL_CONF_STACKSIZE_MAIN];

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
	ipv6_sendto(&bcast, IPV6_PROTO_NUM_NONE, buffer, length);
}

void receive_packet(void) {
	msg_t m;

	while (1) {
		msg_receive(&m);
		DEBUG("got msg from %d, data at %p", m.sender_pid, m.content.ptr);
	}
}

void ip_init(void) {
	DEBUG("sixlowpan_lowpan_init");
	sixlowpan_lowpan_init(_trans_type, 23, 0);

	DEBUG("sixlowpan_lowpan_bootstrapping");
	sixlowpan_lowpan_bootstrapping();

	int pid = thread_create(receive_thread_stack, sizeof receive_thread_stack, PRIORITY_MAIN-1, CREATE_STACKTEST, receive_packet, "receive");
	ipv6_register_packet_handler(pid);

	ipv6_iface_print_addrs();
}

int main(void) {
#ifdef ENABLE_DEBUG
	local_name = strdup("A");
#endif

	node_init();
	ip_init();

	local_addr->_type = AF_INET6;
	local_addr->_prefix_len = 128;
	inet_pton(AF_INET6, "2001::1", local_addr->_addr);

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

	return 0;
}

#endif /* RIOT */
