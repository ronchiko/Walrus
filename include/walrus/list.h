#pragma once

#ifndef LIST_MEMORY_SIZE
#define LIST_MEMORY_SIZE		64 
#endif

typedef struct _Wal_Obj Walrus_Object;

typedef struct _Wal_Node {
	Walrus_Object *object;
	struct _Wal_Node *next, *prev;
} Walrus_Node;

typedef struct _Wal_List {
	Walrus_Node *head, *tail;
	int elements;
	struct {
		size_t elements;
		Walrus_Node **buffer;
	} list_memory;
} Walrus_List;

void Walrus_ListInit(Walrus_List *list);
void Walrus_ListFree(Walrus_List *list); 

void Walrus_ListInsert(Walrus_List *list, Walrus_Object *object);

/** \brief Gets an element in the list by itertating over the a section of the list, by using the index of list */
Walrus_Node *Walrus_ListIndexedGet(Walrus_List *list, int index);
/** \brief Gets an element in the list by itertating over the list */
Walrus_Node *Walrus_ListIterativeGet(Walrus_List *list, int index);
