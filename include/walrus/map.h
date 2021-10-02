#pragma once

#include <stddef.h>
#include <stdint.h>

#include "walrus/list.h"

#ifndef PREALLOCATED_MAP_BUFFER
#define PREALLOCATED_MAP_BUFFER		13
#endif

typedef char *Walrus_MapKey;

uint64_t Walrus_HashString(const char *str, size_t x);

typedef struct {
	int pcl;
	Walrus_MapKey key;
	Walrus_Object *object;

	int pcl_count;
} Walrus_MapItem;

typedef struct _Walrus_Map {
	struct {
		Walrus_MapItem stack[PREALLOCATED_MAP_BUFFER];
		Walrus_MapItem *heap;
	} items;
	size_t size;
	size_t occopied;
} Walrus_Map;

void Walrus_MapInit(Walrus_Map *map);

void Walrus_MapInsert(Walrus_Map *map, const char *key, Walrus_Object *object);
Walrus_Object * Walrus_MapLookup(Walrus_Map *map, const char *key);
void Walrus_MapRemove(Walrus_Map *map, const char *key);

void Walrus_MapFree(Walrus_Map *map);