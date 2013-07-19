#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef RIOT
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#else
#include <vtimer.h>
#include <cc110x_ng.h>
#endif

#include "writer.h"
#include "reader.h"

#include "rfc5444/rfc5444_reader.h"
#include "rfc5444/rfc5444_writer.h"

#ifndef RIOT
int sockfd;
struct sockaddr_in servaddr;
#else
cc110x_packet_t packet;
#endif

void write_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
	struct rfc5444_writer_target *iface __attribute__((unused)),
	void *buffer, size_t length) {

	printf("write_packet called\n");
#ifndef RIOT
	sendto(sockfd, buffer, length, 0,
		(struct sockaddr*) &servaddr, sizeof(servaddr));
#else
	// TODO: no memcpy, set address etc.
	memcpy(packet.data, buffer, length);
	packet.length = length;
	cc110x_send(&packet);
#endif
}

#ifndef RIOT
void init_socket(in_addr_t addr, int port) {
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	memset(&servaddr, 0, sizeof servaddr);
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = addr;
	servaddr.sin_port = htons(port);
}

void sigio_handler(int sig) {
	char buffer[1500];
	int length;

	if ((length = recvfrom(sockfd, &buffer, sizeof buffer, 0, 0, 0)) == -1)
		return;

	reader_handle_packet(buffer, length);
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
#ifndef RIOT
	if (argc != 3) {
		printf("usage:  %s <IP address> <port>\n", argv[0]);
		return -1;
	}

	init_socket(inet_addr(argv[1]), atoi(argv[2]));
	enable_asynch(sockfd);
#endif
	reader_init();
	writer_init(write_packet);

	while (1) {
		sleep_s(5);
		writer_tick();
	}

	reader_cleanup();
	writer_cleanup();
}
