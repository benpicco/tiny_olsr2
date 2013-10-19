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

struct number_list {
	struct number_list* next;
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

	CHECK_TRUE(_is_equal(simple_list_find_cmp(_head, buffer, (int (*)(void *, void *)) strcmp), 42, bar), 
		"%s", _print_result(simple_list_find_cmp(_head, buffer, (int (*)(void *, void *)) strcmp)));

	END_TEST();
}

void _add_number_list(struct number_list** head, int value) {
	struct number_list* node = simple_list_add_before(head, value);
	node->value = value;
}

void test_number_list() {
	struct number_list* head = 0;

	_add_number_list(&head, 23);
	_add_number_list(&head, 42);
	_add_number_list(&head, 17);
	_add_number_list(&head, 32);
	_add_number_list(&head, 1);

	START_TEST();

	int prev = 0;
	struct number_list* node;
	simple_list_for_each (head, node) {
		CHECK_TRUE(node->value >= prev, "%d < %d", node->value, prev);
		prev = node->value;
	}

	END_TEST();
}

void test_for_each_remove() {
	struct number_list* head = 0;

	int i=0;
	int max = 11;
	for (i = 1; i < max; ++i) {
		if (i == 2)
			_add_number_list(&head, 3);
		else
			_add_number_list(&head, i);
	}

	START_TEST();

	char skipped;
	struct number_list *node, *prev;
	simple_list_for_each_safe(head, node, prev, skipped) {
		if (node->value % 2) {
			printf("removing %d\n", node->value);
			simple_list_for_each_remove(&head, node, prev);
		} else
			printf("keeping %d\n", node->value);
	}

	i = 0;
	simple_list_for_each(head, node) {
		CHECK_TRUE(node->value % 2 == 0, "%d", node->value);
		++i;
	}
	CHECK_TRUE(i == max / 2 - 1, "missed an entry");

	simple_list_for_each_safe(head, node, prev, skipped)
		simple_list_for_each_remove(&head, node, prev);

	CHECK_TRUE(head == NULL, "list not cleared properly");

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

	test_number_list();
	test_for_each_remove();

	return FINISH_TESTING();
}
