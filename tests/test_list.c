#include <stdio.h>
#include <string.h>
#include "list.h"
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

struct sorted_list {
	struct sorted_list* next;
	int value;
};

struct test_list* _get_by_buffer(struct test_list* head, char* buffer) {
	return simple_list_find(head, buffer);
}

struct test_list* _get_by_value(struct test_list* head, int value) {
	return simple_list_find(head, value);
}

struct test_list* _add_test_list(struct test_list** head, char* buffer, int value) {
	struct test_list* node = simple_list_add_tail(head);

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

int _is_equal(struct test_list* node, const int value, const char *buffer) {
	return node != 0 && node->value == value && !strcmp(node->buffer, buffer);
}

void test_simple_list_fill(struct test_list* _head) {
	START_TEST();

	CHECK_TRUE(_is_equal(_get_by_buffer(_head, bar), 42, bar), "%s", _print_result(_get_by_buffer(_head, bar)));
	CHECK_TRUE(_is_equal(_get_by_value(_head, 23), 23, foo), "%s", _print_result(_get_by_value(_head, 23)));

	END_TEST();
}

void test_simple_list_remove(struct test_list** __head) {
	struct test_list* _head;
	simple_list_remove(__head, _get_by_buffer(*__head, foo));
	_head = *__head;

	START_TEST();

	CHECK_TRUE(_is_equal(_head, 42, bar), "%s", _print_result(_head));
	CHECK_TRUE(_is_equal(_head->next, 1337, baz), "%s", _print_result(_head->next));

	END_TEST();
}

void test_simple_list_find(struct test_list* _head) {
	char buffer[sizeof bar];
	memcpy(buffer, bar, sizeof buffer);

	START_TEST();

	CHECK_TRUE(_is_equal(simple_list_find_memcmp(_head, buffer), 42, bar), "%s", _print_result(simple_list_find_memcmp(_head, buffer)));

	CHECK_TRUE(_is_equal(simple_list_find_cmp(_head, buffer, strcmp), 42, bar), "%s", _print_result(simple_list_find_cmp(_head, buffer, strcmp)));

	END_TEST();
}

void _add_sorted_list(struct sorted_list** head, int value) {
	struct sorted_list* node = simple_list_add_before(head, value);
	node->value = value;
}

void test_sorted_list() {
	struct sorted_list* head = 0;

	_add_sorted_list(&head, 23);
	_add_sorted_list(&head, 42);
	_add_sorted_list(&head, 17);
	_add_sorted_list(&head, 32);
	_add_sorted_list(&head, 1);

	START_TEST();

	int prev = 0;
	struct sorted_list* node;
	simple_list_for_each (head, node) {
		CHECK_TRUE(node->value >= prev, "%d < %d", node->value, prev);
		prev = node->value;
	}

	END_TEST();
}

int main(void) {
	struct test_list* _head = 0;

	_add_test_list(&_head, foo, 23);
	_add_test_list(&_head, bar, 42);
	_add_test_list(&_head, baz, 1337);

	struct test_list* node;
	simple_list_for_each (_head, node)
		printf("%s\n", _print_result(node));

	BEGIN_TESTING(0);

	test_simple_list_fill(_head);
	test_simple_list_remove(&_head);
	test_simple_list_find(_head);

	test_sorted_list();

	return FINISH_TESTING();
}
