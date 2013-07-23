#include <stdio.h>
#include "../node/list.h"

char foo[] = "Hello World!";
char bar[] = "Hello CUnit!";
char baz[] = "c-c-c-combobreaker";

struct test_list {
	struct test_list* next;
	char* buffer;
	int value;
};

struct test_list* _get_by_buffer(struct test_list* head, char* buffer) {
	return list_find(head, buffer);
}

struct test_list* _get_by_value(struct test_list* head, int value) {
	return list_find(head, value);
}

struct test_list* _add_test_list(struct test_list** head, char* buffer, int value) {
	struct test_list* node = list_add_tail(head);

	node->buffer = buffer;
	node->value  = value;

	return node;
}

void _print_result(struct test_list* result) {
	if (result)
		printf("%d, %s\n", result->value, result->buffer);
	else
		printf("Not found\n");
}

void _print_list(struct test_list* head) {
	while (head) {
		_print_result(head);
		head = head->next;
	}
}

int main(void) {
	struct test_list* _head = 0;

	_add_test_list(&_head, foo, 23);
	_add_test_list(&_head, bar, 42);
	_add_test_list(&_head, baz, 1337);

	_print_list(_head);

	puts("-------------");

	_print_result(_get_by_buffer(_head, bar));
	_print_result(_get_by_value(_head, 23));

	puts("-------------");

	list_remove(&_head, _get_by_buffer(_head, foo));
	_print_list(_head);

	puts("-------------");

	char buffer[sizeof bar];
	memcpy(buffer, bar, sizeof buffer);
	_print_result(list_find_memcmp(_head, buffer));
}