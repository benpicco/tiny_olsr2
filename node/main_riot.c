#ifdef RIOT
#include <thread.h>
#include <vtimer.h>
#include <socket.h>
#include <inet_pton.h>
#include <sixlowpan/ip.h>
#include <msg.h>
#include <net_help.h>
#include <random.h>

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

static int sock;
static sockaddr6_t sa_bcast;

void enable_receive(void) {
	// TODO
}

void disable_receive(void) {
	// TODO
}

void write_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
	struct rfc5444_writer_target *iface __attribute__((unused)),
	void *buffer, size_t length) {

	DEBUG("write_packet(%d bytes)", length);

	int bytes_send = sendto(sock, buffer, length, 0, &sa_bcast, sizeof sa_bcast);

	DEBUG("%d bytes sent", bytes_send);
}

void receive_packet(void) {
	char buffer[256];

	sockaddr6_t sa = {0};
	sa.sin6_family = AF_INET6;
	sa.sin6_port = HTONS(MANET_PORT);

	if (bind(sock, &sa, sizeof sa) < 0) {
		printf("Error bind failed!\n");
		close(sock);
	}

	/* wake this thread when a new UDP packet arrives */
	ipv6_register_next_header_handler(IPV6_PROTO_NUM_UDP, thread_getpid());

	int32_t recsize;
	uint32_t fromlen = sizeof sa;

	char addr_str[IPV6_MAX_ADDR_STR_LEN];

	struct netaddr _src;
	_src._type = AF_INET6;
	_src._prefix_len = 64;

	while (1) {
		recsize = recvfrom(sock, &buffer, sizeof buffer, 0, &sa, &fromlen);
		DEBUG("Received %d bytes from %s\n", recsize, ipv6_addr_to_str(addr_str, &sa.sin6_addr));

		memcpy(&_src._addr, &sa.sin6_addr, sizeof _src._addr);
		reader_handle_packet(&buffer, recsize, &_src);
	}
}

void ip_init(void) {
	uint8_t hw_addr = genrand_uint32() % 128;

	sixlowpan_lowpan_init(_trans_type, hw_addr, 0);

	/* we always send to the same broadcast address, prepare it once */
	sa_bcast.sin6_family = AF_INET6;
	sa_bcast.sin6_port = HTONS(MANET_PORT);
	ipv6_addr_set_all_nodes_addr(&sa_bcast.sin6_addr);

	sock = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);

	thread_create(receive_thread_stack, sizeof receive_thread_stack, PRIORITY_MAIN-1, CREATE_STACKTEST, receive_packet, "receive");

	ipv6_iface_print_addrs();
}

int main(void) {
#ifdef ENABLE_DEBUG
	local_name = strdup("A");
#endif
	genrand_init(time_now());

	node_init();
	ip_init();

	local_addr->_type = AF_INET6;
	local_addr->_prefix_len = 64;
	ipv6_iface_get_best_src_addr(&local_addr->_addr, &sa_bcast.sin6_addr);

	DEBUG("This is node %s with IP %s",
		local_name, netaddr_to_str_s(&nbuf[0], local_addr));

	reader_init();
	writer_init(write_packet);

	while (1) {
		sleep_s(REFRESH_INTERVAL);

		disable_receive();

		remove_expired(0);

//		print_neighbors();
		print_topology_set();
//		print_routing_graph();

		writer_send_hello();
		writer_send_tc();

		DEBUG_TICK;
		enable_receive();
	}

	reader_cleanup();
	writer_cleanup();
	close(sock);

	return 0;
}

#endif /* RIOT */
