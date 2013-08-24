#include <stdio.h>
#include <string.h>
#include "../node/list.h"
#include "cunit/cunit.h"

char foo[] = "Hello World!";
char bar[] = "Hello CUnit!";
char baz[] = "c-c-c-combobreaker";

char print_buffer[128];

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

char* _print_result(struct test_list* result) {
	if (result)
		snprintf(print_buffer, sizeof print_buffer, "%d, %s\n", result->value, result->buffer);
	else
		snprintf(print_buffer, sizeof print_buffer, "Not found\n");

	return print_buffer;
}

void _print_list(struct test_list* head) {
	while (head) {
		printf("%s\n", _print_result(head));
		head = head->next;
	}
}

int _is_equal(struct test_list* node, const int value, const char *buffer) {
	return node != 0 && node->value == value && !strcmp(node->buffer, buffer);
}

void test_list_fill(struct test_list* _head) {
	START_TEST();

	CHECK_TRUE(_is_equal(_get_by_buffer(_head, bar), 42, bar), "%s", _print_result(_get_by_buffer(_head, bar)));
	CHECK_TRUE(_is_equal(_get_by_value(_head, 23), 23, foo), "%s", _print_result(_get_by_value(_head, 23)));

	END_TEST();
}

void test_list_remove(struct test_list** __head) {
	struct test_list* _head;
	list_remove(__head, _get_by_buffer(*__head, foo));
	_head = *__head;

	START_TEST();

	CHECK_TRUE(_is_equal(_head, 42, bar), "%s", _print_result(_head));
	CHECK_TRUE(_is_equal(_head->next, 1337, baz), "%s", _print_result(_head->next));

	END_TEST();
}

void test_list_find(struct test_list* _head) {
	char buffer[sizeof bar];
	memcpy(buffer, bar, sizeof buffer);

	START_TEST();

	CHECK_TRUE(_is_equal(list_find_memcmp(_head, buffer), 42, bar), "%s", _print_result(list_find_memcmp(_head, buffer)));

	END_TEST();
}

int main(void) {
	struct test_list* _head = 0;

	_add_test_list(&_head, foo, 23);
	_add_test_list(&_head, bar, 42);
	_add_test_list(&_head, baz, 1337);

	_print_list(_head);

	BEGIN_TESTING(0);

	test_list_fill(_head);
	test_list_remove(&_head);
	test_list_find(_head);

	return FINISH_TESTING();
}
