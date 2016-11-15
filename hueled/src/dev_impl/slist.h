#ifndef __SLIST_H__
#define __SLIST_H__

slist_element_t* slist_get_last(slist_element_t* first);
int slist_init(slist_t* lst, void (*delete_element)(void*));
slist_element_t* slist_add(slist_t* lst, void* data);
void slist_remove(slist_t* lst, void* data);
void slist_clear(slist_t* lst);
int slist_length(slist_t* lst);
#endif
