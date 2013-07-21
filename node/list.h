#ifndef LIST_H_
#define LIST_H_
#include <stddef.h>

struct list_elem;

void* add_tail(struct list_elem** head, size_t size);
void* find_list(struct list_elem* head, void* needle, int offset);
void list_remove(struct list_elem** head, struct list_elem* node);
#endif /* LIST_H_ */