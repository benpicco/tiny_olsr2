#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

typedef int bool;
#define true	1
#define false	0

struct node {
	int id;
	struct sockaddr_in addr;
	socklen_t addr_len;

	struct connection* connections;
	struct node* next;
};

struct connection {
	struct node* node;
	struct connection* next;
};

static struct node* node_head = 0;

static void connect_node(struct node* node_a, struct node* node_b, bool bidirectional) {
	if (bidirectional)
		connect_node(node_b, node_a, false);

	printf("%d -> %d\n", node_a->id, node_b->id);

	if (node_a->connections == 0) {
		node_a->connections = malloc(sizeof (struct connection));
		node_a->connections->node = node_b;
		node_a->connections->next = 0;
	} else {
		struct connection* con = node_a->connections;
		while (con->next)
			con = con->next;
		con->next = malloc(sizeof (struct connection));
		con->next->node = node_b;
		con->next->next = 0;
	}
}

static struct node* add_node(int id) {
	struct node* old_head = node_head;
	node_head = malloc(sizeof(struct node));
	node_head->next = old_head;
	node_head->id = id;

	return node_head;
}

static int add_node_data(struct sockaddr_in addr, socklen_t len) {
	struct node *head = node_head;
	do {
		if (head->addr.sin_port == 0) {
			head->addr = addr;
			head->addr_len = len;

			return head->id;
		}
	} while ((head = head->next));

	return -1;
}

static int get_id(struct sockaddr_in addr) {
	struct node *head = node_head;

	do {
		if (head->addr.sin_port == addr.sin_port &&
			head->addr.sin_addr.s_addr == addr.sin_addr.s_addr)
			return head->id;
	} while ((head = head->next));

	return -1;
}

static void write_packet(int id, int socket, void *buffer, size_t length) {
//	printf("[node %d sending %d byte]\n", id, length);

	struct node* head = node_head;
	while (head) {
		if (head->id == id && head->addr.sin_port) {
			struct connection* con = head->connections;
			while (con) {
				if (con->node->addr.sin_port)
					sendto(socket, buffer, length, 0, (struct sockaddr*) &con->node->addr, con->node->addr_len);
				con = con->next;
			}
			return;
		}
		head = head->next;
	}

	printf("Error: can't write package - node not found\n");
}

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

	int id = 0;
	struct node* A = add_node(++id);
	struct node* B = add_node(++id);
	struct node* C = add_node(++id);

	connect_node(A, B, true);
	connect_node(A, C, true);

	if ((socket = setup_socket(atoi(argv[1]))) < 0)
		return -1;

	struct sockaddr_in si_other;
	int n;
	socklen_t slen = sizeof(si_other);
	while ((n = recvfrom(socket, buffer, sizeof buffer, 0, (struct sockaddr*) &si_other, &slen))) {
		int id = get_id(si_other);
		if (id < 0)
			id = add_node_data(si_other, slen);
		if (id < 0)
			printf("ignoring unknown node");
		else
			write_packet(get_id(si_other), socket, buffer, n);
	}

	close(socket);
	return 0;
}
