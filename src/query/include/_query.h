#pragma once

#include <walrus/query.h>

#define QUERY_ITEM_LENGTH NAME_BUFFER_SIZE << 1

typedef struct { 
	struct {
		bool isDynamic;
		Walrus_Object **results;
		size_t limit, count;
	} results;
	const char *query;
} Walrus_QueryContext;

typedef bool (*_Walrus_QueryHandler)(Walrus_Object *, Walrus_Stream *, Walrus_QueryContext *);

bool Walrus_PushQueryResult(Walrus_QueryContext *, Walrus_Object *result);
bool Walrus_PerformQuery(Walrus_Object *root, Walrus_Stream *stream, Walrus_QueryContext *result);

// Branching
bool Walrus_QueryBranchOnObject(Walrus_Object *root, Walrus_Stream *stream, Walrus_QueryContext *result);
bool Walrus_QueryBranchOnList(Walrus_Object *root, Walrus_Stream *stream, Walrus_QueryContext *result);

// Getters
bool Walrus_QueryGetProperty(Walrus_Object *root, Walrus_Stream *stream, Walrus_QueryContext *result);
bool Walrus_QueryGetListItem(Walrus_Object *root, Walrus_Stream *stream, Walrus_QueryContext *result);

// Conditions
bool Walrus_QueryTypeMatch(Walrus_Object *root, Walrus_Stream *stream, Walrus_QueryContext *result);
