#include <stdlib.h>

#include "list.h"

struct list_elem {
	struct list_elem* next;
};

void* add_tail(struct list_elem** head, size_t size) {
	struct list_elem* _head = *head;

	if (!_head)
		return *head = malloc(size);

	while (_head->next) {
		_head = _head->next;
	}

	return _head->next = malloc(size);
}

void* _find_list(struct list_elem* head, void* needle, int offset) {
	while (head) {
		void** buff = (void*) head + offset;

		if (*buff ==  needle)
			return head;
		head = head->next;
	}

	return 0;
}

void list_remove(struct list_elem** head, struct list_elem* node) {
	struct list_elem* _head = *head;
	struct list_elem* prev = 0;

	while (_head && _head != node) {
		prev = _head;
		_head = _head->next;
	}

	if (_head != node)
		return; // not found

	if (!prev)	// remove head
		*head = _head->next;
	else
		prev->next = node->next;

	free(node);
}