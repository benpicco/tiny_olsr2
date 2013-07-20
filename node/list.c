#include <stdlib.h>

#include "list.h"

struct list_elem {
	struct list_elem* next;
};

void* add_tail(struct list_elem* head, size_t size) {
	while (head) {
		head = head->next;
	}

	return head = malloc(size);
}

void list_remove(struct list_elem* head, struct list_elem* node) {
	struct list_elem* prev = 0;
	while (head && head != node) {
		prev = head;
		head = head->next;
	}

	if (head != node)
		return; // not found

	prev->next = node->next;
	free(node);
}