#include <stdlib.h>

#include "list.h"

struct list_elem {
	struct list_elem* next;
};

void* _list_add_tail(struct list_elem** head, size_t size) {
	struct list_elem* _head = *head;

	if (!_head) {
		*head = calloc(size, 0);
		return *head;
	}

	while (_head->next) {
		_head = _head->next;
	}

	_head = _head->next = calloc(size, 0);
	return _head;
}

void* _list_add_head(struct list_elem** head, size_t size) {
	struct list_elem* _head = *head;

	*head = calloc(size, 1);
	(*head)->next = _head;

	return *head;
}

void* _list_find(struct list_elem* head, void* needle, int offset, size_t size) {
	while (head) {
		void** buff = (void*) head + offset;

		if (size == 0 && *buff == needle)
			return head;
		if (size > 0 && !memcmp(*buff, needle, size))
			return head;
		head = head->next;
	}

	return 0;
}

void _list_remove(struct list_elem** head, struct list_elem* node) {
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