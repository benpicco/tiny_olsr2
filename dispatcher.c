#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static int setup_socket(int port) {
	struct sockaddr_in si_me;
	int s;

	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		printf("Can't create socket");
		return -2;
	}
	
	memset(&si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(port);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(s, (struct sockaddr*) &si_me, sizeof(si_me)) < 0) {
	    printf("Can't bind socket\n");
	    return -2;
	}

	return s;
}

int main(int argc, char** argv) {
	int socket;
	char buffer[1500];

	if (argc != 2) {
    	printf("usage:  %s <port>\n", argv[0]);
    	return -1;
	}

	if ((socket = setup_socket(atoi(argv[1]))) < 0)
		return -1;

	struct sockaddr_in si_other;	
	int n, slen = sizeof(si_other);
	while (n = recvfrom(socket, buffer, sizeof buffer, 0, (struct sockaddr*) &si_other, &slen)) {
	    buffer[n] = 0;
	    printf("Data: %s\n", buffer);
	  	printf("Received packet from %d\n", ntohs(si_other.sin_port));

	  	sendto(socket, buffer, n, 0, (struct sockaddr*) &si_other, slen);
	}
	
	close(socket);
	return 0;
}
