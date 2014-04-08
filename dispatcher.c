#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "fake_metric.h"

struct node {
	char* name;
	struct sockaddr_in addr;
	socklen_t addr_len;

	struct connection* connections;
	struct node* next;
};

struct connection {
	struct node* node;
	struct connection* next;
	float loss;
};

static struct node* node_head = 0;

static void connect_node(struct node* node_a, struct node* node_b, float loss, bool bidirectional) {
	if (bidirectional)
		connect_node(node_b, node_a, loss, false);

	printf("%s -> %s (loss: %.2f)\n", node_a->name, node_b->name, loss);

	if (node_a->connections == 0) {
		node_a->connections = malloc(sizeof (struct connection));
		node_a->connections->node = node_b;
		node_a->connections->next = 0;
		node_a->connections->loss = loss;
	} else {
		struct connection* con = node_a->connections;
		while (con->next)
			con = con->next;
		con->next = malloc(sizeof (struct connection));
		con->next->node = node_b;
		con->next->next = 0;
		con->next->loss = loss;
	}
}

static struct node* find_node(const char* name) {
	struct node* n = node_head;
	while (n) {
		if (!strcmp(n->name, name))
			return n;
		n = n->next;
	}
	return 0;
}

static struct node* add_node(const char* name) {
	struct node* old_head = node_head;

	struct node* n = find_node(name);
	if (n)
		return n;

	node_head = malloc(sizeof(struct node));
	node_head->next = old_head;
	node_head->name = strdup(name);

	return node_head;
}

static struct node* add_node_data(struct sockaddr_in addr, socklen_t len) {
	struct node *head = node_head;
	do {
		if (head->addr.sin_port == 0) {
			head->addr = addr;
			head->addr_len = len;

			return head;
		}
	} while ((head = head->next));

	return 0;
}

static struct node* get_node(struct sockaddr_in addr) {
	struct node *head = node_head;

	do {
		if (head->addr.sin_port == addr.sin_port &&
			head->addr.sin_addr.s_addr == addr.sin_addr.s_addr)
			return head;
	} while ((head = head->next));

	return 0;
}

static void write_packet(struct node* n, int socket, void *buffer, size_t length) {
	printf("%snode %s sending %zd byte\t", time(0) % 2 ? "\t" : "" , n->name, length);

	struct packet* ether = buffer;

	struct connection* con = n->connections;
	while (con) {
		if (con->node->addr.sin_port && (con->loss * RAND_MAX < random())) {
			if (ether->magic == METRIC_MAGIC)
				ether->metric = con->loss ? con->loss * METRIC_MAX : 1;

			sendto(socket, buffer, length, 0, (struct sockaddr*) &con->node->addr, con->node->addr_len);
			printf(".");
		} else
			printf("!");
		con = con->next;
	}
	printf("\n");
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
	FILE* fp;
	char buffer[1500];

	if (argc != 3) {
    	printf("usage:  %s <file> <port>\n", argv[0]);
    	return -1;
	}

	if ((fp = fopen(argv[1],"r")) == NULL) {
		printf("Cannot open file.\n");
    	return -2;
    }

	char a[64];
	char b[64];
	float loss;

	int matches = 0;
	bool bidirectional = false;
	while (EOF != (matches = fscanf(fp, "%s -> %s\t[label = %f]\n", a, b, &loss))) {
		if (matches == 2)
			connect_node(add_node(a), add_node(b), 0, bidirectional);
		else if (matches == 3)
			connect_node(add_node(a), add_node(b), loss, bidirectional);
		else {
			if (!strncmp(a, "bidirectional", strlen("bidirectional")))
				bidirectional = true;
			if (!strncmp(a, "directional", strlen("directional")))
				bidirectional = false;
		}
	}

	fclose(fp);

	if ((socket = setup_socket(atoi(argv[2]))) < 0)
		return -1;

	struct sockaddr_in si_other;
	size_t size;
	socklen_t slen = sizeof(si_other);
	while ((size = recvfrom(socket, buffer, sizeof buffer, 0, (struct sockaddr*) &si_other, &slen))) {
		struct node* n = get_node(si_other);
		if (n == 0) {
			n = add_node_data(si_other, slen);
			if (n)
				sendto(socket, n->name, strlen(n->name), 0, (struct sockaddr*) &n->addr, n->addr_len);
		}

		if (n == 0)
			printf("ignoring unknown node\n");
		else
			write_packet(n, socket, buffer, size);
	}

	close(socket);
	return 0;
}
