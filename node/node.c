#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

#include "writer.h"
#include "reader.h"

#include "rfc5444/rfc5444_reader.h"
#include "rfc5444/rfc5444_writer.h"

int sockfd;
struct sockaddr_in servaddr;

void write_packet(struct rfc5444_writer *wr __attribute__ ((unused)), 
	struct rfc5444_writer_target *iface __attribute__((unused)), 
	void *buffer, size_t length) {

	printf("write_packet called\n");
	sendto(sockfd, buffer, length, 0,
		(struct sockaddr*) &servaddr, sizeof(servaddr));
}

void init_socket(in_addr_t addr, int port) {
   sockfd = socket(AF_INET, SOCK_DGRAM, 0);

   memset(&servaddr, 0, sizeof servaddr);
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr = addr;
   servaddr.sin_port = htons(port);
}

int main(int argc, char** argv) {
	if (argc != 3) {
		printf("usage:  %s <IP address> <port>\n", argv[0]);
		return -1;
	}

	init_socket(inet_addr(argv[1]), atoi(argv[2]));

	reader_init();
	writer_init(write_packet);

	writer_tick();

	reader_cleanup();
	writer_cleanup();
}