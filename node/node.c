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

#include "nhdp_writer.h"
#include "nhdp_reader.h"
#include "nhdp.h"

#include "rfc5444/rfc5444_print.h"
#include "rfc5444/rfc5444_reader.h"
#include "rfc5444/rfc5444_writer.h"

#ifdef RIOT
cc110x_packet_t packet;
#else
int sockfd;
struct sockaddr_in servaddr;
#endif

/* for hexfump */
static struct autobuf _hexbuf;

void write_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
	struct rfc5444_writer_target *iface __attribute__((unused)),
	void *buffer, size_t length) {

	puts("write_packet()");

	/* generate hexdump of packet */
	abuf_hexdump(&_hexbuf, "\t", buffer, length);
	rfc5444_print_direct(&_hexbuf, buffer, length);

	/* print hexdump to console */
	printf("%s", abuf_getptr(&_hexbuf));

	abuf_clear(&_hexbuf);
#ifdef RIOT
	// TODO: no memcpy, set address etc.
	memcpy(packet.data, buffer, length);
	packet.length = length;
	cc110x_send(&packet);
#else
	sendto(sockfd, buffer, length, 0,
		(struct sockaddr*) &servaddr, sizeof(servaddr));
#endif
}

#ifndef RIOT
void sigio_handler(int sig) {
	char buffer[1500];
	int length;

	if ((length = recvfrom(sockfd, &buffer, sizeof buffer, 0, 0, 0)) == -1)
		return;

	nhdp_reader_handle_packet(buffer, length);
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

	/* send HELLO */
	sendto(sockfd, this_ip, strlen(this_ip), 0, (struct sockaddr*) &servaddr, sizeof(servaddr));
	/* get our name */
	char this_name[32];
	size_t size = recvfrom(sockfd, this_name, sizeof this_name, 0, 0, 0);
	this_name[size] = 0;

	printf("This is node %s\n", this_name);
#ifdef DEBUG
	node_name = strdup(this_name);
#endif

	enable_asynch(sockfd);
#endif

	inet_pton(AF_INET6, this_ip, local_addr._addr);
	local_addr._type = AF_INET6;
	local_addr._prefix_len = 128;

	nhdp_reader_init();
	nhdp_writer_init(write_packet);

	while (1) {
		sleep_s(5);
		nhdp_writer_tick();
	}

	nhdp_reader_cleanup();
	nhdp_writer_cleanup();
	abuf_free(&_hexbuf);
}
