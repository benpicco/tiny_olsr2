#include <stdio.h>
#include <vtimer.h>
#include <thread.h>
#include <net_help.h>
#include <destiny/socket.h>

#include "udp_ping.h"

#define PING_PORT       25431

static char pong_stack[KERNEL_CONF_STACKSIZE_DEFAULT];

static int sock, id, tid;

void(*_ping_callback)(struct ping_packet*);

static void pong(void) {
	struct ping_packet pong;

	sockaddr6_t sa = {0};
	sa.sin6_family = AF_INET6;
	sa.sin6_port = HTONS(PING_PORT);

	if (destiny_socket_bind(sock, &sa, sizeof sa) < 0) {
		printf("Error bind failed!\n");
		destiny_socket_close(sock);
	}

	int32_t recsize;
	uint32_t fromlen = sizeof sa;

	while (id) {
		recsize = destiny_socket_recvfrom(sock, &pong, sizeof pong, 0, &sa, &fromlen);
		if (pong.received)
			_ping_callback(&pong);
		else {
			pong.hops = 23;	// TODO
			pong.received = 1;
			sa.sin6_port = HTONS(PING_PORT);
			destiny_socket_sendto(sock, &pong, recsize, 0, &sa, fromlen);
		}
	}
}

void ping_start_listen(void(*callback)(struct ping_packet*)) {
	_ping_callback = callback;
	id = 1;
	sock = destiny_socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	tid = thread_create(pong_stack, sizeof pong_stack, PRIORITY_MAIN-1, CREATE_STACKTEST, pong, "pong");
}

void ping_stop_listen(void) {
	id = 0;
	thread_wakeup(tid);
	destiny_socket_close(sock);
}

void ping_send(ipv6_addr_t* dest) {
	timex_t now;
	vtimer_now(&now);

	struct ping_packet ping;
	ping.hops = 0;
	ping.received = 0;
	ping.id = id++;
	ping.timestamp = now.microseconds;

	sockaddr6_t sa = {0};
	sa.sin6_family = AF_INET6;
	sa.sin6_port = HTONS(PING_PORT);
	memcpy(&sa.sin6_addr, dest, sizeof(ipv6_addr_t));

	destiny_socket_sendto(sock, &ping, sizeof ping, 0, &sa, sizeof sa);
}
