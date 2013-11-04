#include <stdlib.h>

#include "list.h"

struct simple_list_elem {
	struct simple_list_elem* next;
};

void* _simple_list_add_tail(struct simple_list_elem** head, size_t size) {
	struct simple_list_elem* _head = *head;
	void* mem;

	/* check out-of-memory condition */
	if ((mem = calloc(1, size)) == NULL)
		return NULL;

	if (!_head) {
		*head = mem;
		return *head;
	}

	while (_head->next) {
		_head = _head->next;
	}

	_head = _head->next = mem;
	return _head;
}

void* _simple_list_add_head(struct simple_list_elem** head, size_t size) {
	struct simple_list_elem* _head = *head;
	void* mem;

	/* check out-of-memory condition */
	if ((mem = calloc(1, size)) == NULL)
		return NULL;

	*head = mem;
	(*head)->next = _head;

	return *head;
}

void* _simple_list_add_before(struct simple_list_elem** head, size_t size, int needle, int offset) {
	struct simple_list_elem* _head = *head;
	struct simple_list_elem* prev = 0;
	void* mem;

	/* check out-of-memory condition */
	if ((mem = calloc(1, size)) == NULL)
		return NULL;

	if (!_head) {
		*head = mem;
		return *head;
	}

	while(_head) {
		int* buff = (void*) _head + offset;
		if (*buff > needle) {
			if (prev) {
				prev->next = mem;
				prev->next->next = _head;
				return prev->next;
			}

			prev = mem;
			prev->next = _head;
			*head = prev;
			return prev;
		}
		prev = _head;
		_head = _head->next;
	}

	_head = prev->next = mem;
	return _head;
}

void* _simple_list_find(struct simple_list_elem* head, void* needle, int offset, size_t size) {
	while (head) {
		void** buff = (void*) head + offset;

		if (size == 0 && *buff == needle)
			return head;
		if (size > 0 && memcmp(*buff, needle, size) == 0)
			return head;
		head = head->next;
	}

	return 0;
}

void* _simple_list_find_cmp(struct simple_list_elem* head, void* needle, int offset, int compare(void*, void*)) {
	while (head) {
		void** buff = (void*) head + offset;

		if (compare(*buff, needle) == 0)
			return head;

		head = head->next;
	}

	return 0;
}

bool _simple_list_remove(struct simple_list_elem** head, struct simple_list_elem* node) {
	struct simple_list_elem* _head = *head;
	struct simple_list_elem* prev = 0;

	while (_head && _head != node) {
		prev = _head;
		_head = _head->next;
	}

	if (_head != node)
		return false; // not found

	if (!prev)	// remove head
		*head = _head->next;
	else
		prev->next = node->next;

	free(node);
	return true;
}