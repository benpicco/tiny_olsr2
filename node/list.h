#ifndef LIST_H_
#define LIST_H_
#include <stddef.h>

struct list_elem;

#define list_add_tail(head)	_list_add_tail((struct list_elem**) head, sizeof **head)
#define list_find(head, value)	_list_find((struct list_elem*) head, value, (void*) &head->value - (void*) head)
#define list_remove(head, node) _list_remove((struct list_elem**) head, (struct list_elem*) node)

void* _list_add_tail(struct list_elem** head, size_t size);
void* _list_find(struct list_elem* head, void* needle, int offset);
void  _list_remove(struct list_elem** head, struct list_elem* node);
#endif /* LIST_H_ */