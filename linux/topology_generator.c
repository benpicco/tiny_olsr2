#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "list.h"

#define BUF_SIZE 8

struct node {
	struct node* next;
	char* name;
};

char* itoname(unsigned int n) {
	char tmp[2];
	char* dest = calloc(sizeof(char), BUF_SIZE);
	do {
		sprintf(tmp, "%c", 'A' + n % ('Z' - 'A'));
		strcat(dest, tmp);
		n /= 'Z' - 'A';
	} while (n != 0);

	return dest;
}

void generate_nodes(struct node** h, unsigned int count) {
	for (unsigned int i = 0; i < count; ++i) {
		struct node* n = simple_list_add_head(h);
		n->name = itoname(i);
	}
}

void generate_links(struct node* h) {
	struct node* n;
	simple_list_for_each(h, n) {
		if (n != h && random() < RAND_MAX/4)
			printf("\t%s -> %s\n", h->name, n->name);
	}
}

int main(int argc, char** argv) {
	struct node *head = NULL, *node;

	if (argc < 2) {
		printf("usage:	%s [number]\n", argv[0]);
		return -1;
	}

	srandom(time(0));
	generate_nodes(&head, atoi(argv[1]));

	puts("digraph {");
	puts("subgraph bidirectional {");
	puts("	edge [arrowhead = none]");

	simple_list_for_each(head, node) {
		if (random() < RAND_MAX/4)
			generate_links(node);
	}

	puts("}");

	return 0;
}
