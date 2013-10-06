#ifndef LIST_H_
#define LIST_H_
#include <stddef.h>
#include <string.h>

struct list_elem;

#define list_add_head(head)	_list_add_head((struct list_elem**) head, sizeof **head)
#define list_add_tail(head)	_list_add_tail((struct list_elem**) head, sizeof **head)
#define list_add_before(head, value) *head == 0 ? list_add_head(head) : \
	_list_add_before((struct list_elem**) head, sizeof **head, value, (void*) &(*head)->value - (void*) *head)
#define list_find(head, value)	head == 0 ? NULL : \
	_list_find((struct list_elem*) head, value, (void*) &head->value - (void*) head, 0)
#define list_find_memcmp(head, value)	head == 0 ? NULL : \
	_list_find((struct list_elem*) head, value, (void*) &head->value - (void*) head, sizeof(*value))
#define list_remove(head, node)	_list_remove((struct list_elem**) head, (struct list_elem*) node)

#define list_for_each(head, node)	for(node = head; node; node = node->next)

void* _list_add_head(struct list_elem** head, size_t size);
void* _list_add_tail(struct list_elem** head, size_t size);
void* _list_add_before(struct list_elem** head, size_t size, int needle, int offset);
void* _list_find(struct list_elem* head, void* needle, int offset, size_t size);
void  _list_remove(struct list_elem** head, struct list_elem* node);
#endif /* LIST_H_ */