
#include <stdlib.h>
#include <stdio.h>

#include <walrus/list.h>
#include <walrus/parsing.h>

void Walrus_ListInit(Walrus_List *list) {
	list->list_memory.elements = list->elements = 0;
	list->head = list->tail = NULL;
	list->list_memory.buffer = NULL;
}

void Walrus_ListFree(Walrus_List *list) {

	for(Walrus_Node *tail = list->tail; tail; tail = tail->prev) {
		Walrus_FreeObject(tail->object);
		free(tail->next);
	}

	free(list->head);
	list->elements = 0;
}

void Walrus_ListInsert(Walrus_List *list, Walrus_Object *object) {
	Walrus_Node *node = malloc(sizeof(Walrus_Node));
	node->next = NULL;
	node->prev = list->tail;
	node->object = object;

	if(!list->head)
		list->tail = list->head = node;
	else
		list->tail = list->tail->next = node;

	++list->elements;
	if (!(list->elements % LIST_MEMORY_SIZE)) {
		list->list_memory.buffer = realloc(list->list_memory.buffer, (++list->list_memory.elements) * sizeof(Walrus_Node *));
		list->list_memory.buffer[list->list_memory.elements - 1] = node;
	}
}

Walrus_Node *Walrus_ListIndexedGet(Walrus_List *list, int index) {
	if(index < 0) return NULL;
	int normalized_index = index / LIST_MEMORY_SIZE;
	
	// Lookup in array
	Walrus_Node *begin = list->head;
	if(normalized_index >= list->list_memory.elements + 1) return NULL;

	if (normalized_index)
		begin = list->list_memory.buffer[normalized_index - 1]->next;
	
	Walrus_List _emulated_list = { .head = begin };
	return Walrus_ListIterativeGet(&_emulated_list, index - normalized_index * LIST_MEMORY_SIZE );
}

Walrus_Node *Walrus_ListIterativeGet(Walrus_List *list, int index) {
	if (index < 0) return NULL;

	Walrus_Node *begin = list->head;
	for(int i = 0; begin && i < index; ++i)
		begin = begin->next;
	
	return begin;
}

void _DumpList_(Walrus_List *list) {
	for(Walrus_Node *node = list->head; node; node = node->next){
		printf("%p -> ", node->object);
	}
	printf("(end)\n");
}