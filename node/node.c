#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef RIOT
#include <vtimer.h>
#include <cc110x_ng.h>
#include <destiny/socket.h>
#include <net_help/inet_pton.h>
#else
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif

#include "nhdp.h"
#include "olsr.h"
#include "writer.h"
#include "reader.h"
#include "debug.h"

#include "rfc5444/rfc5444_print.h"
#include "rfc5444/rfc5444_reader.h"
#include "rfc5444/rfc5444_writer.h"

#ifdef RIOT
cc110x_packet_t packet;
#else
int sockfd;
struct sockaddr_in servaddr;

struct ip_lite {
	struct netaddr src;
	size_t length;
};
#endif

/* for hexfump */
static struct autobuf _hexbuf;

void write_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
	struct rfc5444_writer_target *iface __attribute__((unused)),
	void *buffer, size_t length) {

	DEBUG("write_packet(%zd bytes)", length);

	/* generate hexdump of packet */
	abuf_hexdump(&_hexbuf, "\t", buffer, length);
	rfc5444_print_direct(&_hexbuf, buffer, length);

	/* print hexdump to console */
	puts(abuf_getptr(&_hexbuf));

	abuf_clear(&_hexbuf);
#ifdef RIOT
	// TODO: no memcpy, set address etc.
	memcpy(packet.data, buffer, length);
	packet.length = length;
	cc110x_send(&packet);
#else

	struct ip_lite* new_buffer = malloc(sizeof(struct ip_lite) + length);
	memcpy(new_buffer + 1, buffer, length);
	memcpy(&new_buffer->src, &local_addr, sizeof(struct netaddr));
	new_buffer->length = length;

	sendto(sockfd, new_buffer, sizeof(struct ip_lite) + new_buffer->length, 0,
		(struct sockaddr*) &servaddr, sizeof(servaddr));
#endif
}

#ifndef RIOT

void sigio_handler(int sig) {
	char buffer[1500];

	struct sockaddr_storage sender;
	socklen_t sendsize = sizeof(sender);

	if (recvfrom(sockfd, &buffer, sizeof buffer, 0, (struct sockaddr*)&sender, &sendsize) == -1)
		return;

	struct ip_lite* header = (struct ip_lite*) &buffer;
	reader_handle_packet(header + 1, header->length, &header->src);
}

void init_socket(in_addr_t addr, int port) {
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	memset(&servaddr, 0, sizeof servaddr);
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = addr;
	servaddr.sin_port = htons(port);
}

int enable_asynch(int sock) {
	int flags;
	struct sigaction sa;

	flags = fcntl(sock, F_GETFL);
	fcntl(sock, F_SETFL, flags | O_ASYNC);

	sa.sa_flags = 0;
	sa.sa_handler = sigio_handler;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGIO, &sa, NULL))
		return -1;

	if (fcntl(sock, F_SETOWN, getpid()) < 0)
		return -1;

	if (fcntl(sock, F_SETSIG, SIGIO) < 0)
		return -1;

	return 0;
}
#endif

void sleep_s(int secs) {
#ifdef RIOT
	vtimer_usleep(secs * 1000000);
#else
	// process wakes up when a package arrives
	// go back to sleep to prevent flooding
	int remaining_sleep = secs;
	while ((remaining_sleep = sleep(remaining_sleep)));
#endif
}

int main(int argc, char** argv) {
	const char* this_ip;

	/* initialize buffer for hexdump */
	abuf_init(&_hexbuf);

#ifdef RIOT
	cc110x_init(0);	// transceiver_pid ??
	this_ip = "2001::1";
#else
	if (argc != 4) {
		printf("usage:  %s <server IP address> <port> <node IP6 address>\n", argv[0]);
		return -1;
	}
	
	this_ip = argv[3];

	init_socket(inet_addr(argv[1]), atoi(argv[2]));

	/* set timeout for name probing */
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	DEBUG("probing for name…");
	/* send HELLO */
	sendto(sockfd, this_ip, strlen(this_ip), 0, (struct sockaddr*) &servaddr, sizeof(servaddr));
	/* get our name */
	char this_name[32];
	ssize_t size = recvfrom(sockfd, this_name, sizeof this_name, 0, 0, 0);
	if (size < 0) {
		strcpy(this_name, "(null)");
	} else
		this_name[size] = 0;

	DEBUG("This is node %s", this_name);
#ifdef ENABLE_DEBUG
	node_name = strdup(this_name);
#endif

	sigset_t block_io;

	/* Initialize the signal mask. */
	sigemptyset (&block_io);
	sigaddset (&block_io, SIGIO);

	enable_asynch(sockfd);
#endif

	inet_pton(AF_INET6, this_ip, local_addr._addr);
	local_addr._type = AF_INET6;
	local_addr._prefix_len = 128;

	nhdp_init();
	olsr_init();
	reader_init();
	writer_init(write_packet);
	
	while (1) {
		sleep_s(5);

		sigprocmask (SIG_BLOCK, &block_io, NULL);	/* prevent 'interupts' from happening */

		print_neighbors();
		writer_send_hello();
		print_topology_set();
#ifdef ENABLE_DEBUG
		if (*node_name == 'A')
#endif
			writer_send_tc();

		DEBUG_TICK;
		sigprocmask (SIG_UNBLOCK, &block_io, NULL);
	}

	reader_cleanup();
	writer_cleanup();
	abuf_free(&_hexbuf);
}
