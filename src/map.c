
#include <stdlib.h>
#include <string.h>

#include <walrus/parsing.h>
#include <walrus/map.h>

#define MAP_GROWTH_RATE(size) 	(int)((size) * 1.5f)
#define R 						29

#define WALRUS_MAX(a, b) ((a) > (b) ? (a) : (b))
#define WALRUS_MIN(a, b) ((a) > (b) ? (b) : (a))

uint64_t Walrus_HashString(const char *key, size_t length) {
	uint64_t hash = 0;
	for(;*key;++key)
		hash = (R * hash + *key) % length;
	
	return hash;
}

static bool Walrus_IsPrime(size_t n) {
	if (n <= 3) return n > 1;

	if(!(n % 2) || !(n % 3)) return false;

	for (int i = 5; i * i <= n; i += 6) {
		if (n % i == 0 || n % (i + 2) == 0) return false;
	}

	return true;
}

static int around(int a, int b, int source) {
	int value = a - b;
	if (value < 0) return source + value;
	return value;
}

size_t Walrus_NextPrime(size_t prime) {	
	++prime;
	for (; !Walrus_IsPrime(prime); ++prime);
	return prime;
}

void Walrus_MapInit(Walrus_Map *map) {
	map->size = PREALLOCATED_MAP_BUFFER;
	map->occopied = 0;
	map->items.heap = map->items.stack;
	memset(map->items.stack, 0, sizeof(Walrus_MapItem) * PREALLOCATED_MAP_BUFFER);
}

static bool __Walrus_MapInsertUnchecked(Walrus_MapItem *items, size_t length, const char *key, Walrus_Object *value, bool dupkey) {

	char *_key = dupkey ? strdup(key) : (char *)key;
	Walrus_MapItem current = { 0, _key, value, 1 };

	const uint64_t hash = Walrus_HashString(_key, length);
	++items[hash].pcl_count;

	for(int offset = 0; offset < length; ++offset) {
		
		size_t currentIndex = (hash + offset) % length;
		if(!items[currentIndex].key) {
			items[currentIndex] = current;
			return true;
		}
		
		if(offset == items[currentIndex].pcl && strcmp(items[currentIndex].key, key) == 0) {
			// Duplicate key
			Walrus_RaiseError(WALRUS_ERR_DUPLICATE_KEY);
			Walrus_ErrorPushParam(0, strdup(key), true);
			return false;
		} 

		if(current.pcl > items[currentIndex].pcl) {
			Walrus_MapItem temp = current;
			current = items[currentIndex];
			items[currentIndex] = temp;
		}

		++current.pcl;
	}

	return false;
}

void Walrus_MapInsert(Walrus_Map *map, const char *key, Walrus_Object *object) {
	if(map->size <= map->occopied) {
		
		const size_t newSize = Walrus_NextPrime(map->size);
		Walrus_MapItem *items = malloc(newSize * sizeof(Walrus_MapItem));

		// Rehash all items
		memset(items, 0, newSize * sizeof(Walrus_MapItem));
		for(int i = 0; i < map->size; i++) {
			const Walrus_MapItem *current = &map->items.heap[i];

			if (current->key)
				__Walrus_MapInsertUnchecked(items, newSize, current->key, current->object, false);	
		}

		// Only free if its not the stack buffer
		if(map->size > PREALLOCATED_MAP_BUFFER) {
			printf("Freeing items\n");
			free(map->items.heap);
		}

		map->items.heap = items;
		map->size = newSize;
	}

	if(__Walrus_MapInsertUnchecked(map->items.heap, map->size, key, object, true))
		++map->occopied;
}

Walrus_Object *Walrus_MapLookup(Walrus_Map *map, const char *key) {

	const uint64_t hash = Walrus_HashString(key, map->size);
	int pcls = map->items.heap[hash].pcl_count;
	float times = pcls * .5f;

	for(int i = 0; i <= times; ++i) {
		Walrus_MapItem *left  = &map->items.heap[around(hash, i, map->size) % map->size],
					   *right = &map->items.heap[(hash + i) % map->size];

		if (left->key && strcmp(left->key, key) == 0) return left->object;
		if (right->key && strcmp(right->key, key) == 0) return right->object;
	}

	return NULL;
}

static Walrus_MapItem *Walrus_MapProbe(Walrus_Map *map, const char *key) {
	const uint64_t hash = Walrus_HashString(key, map->size);
	int pcls = map->items.heap[hash].pcl_count;
	for(int offset = 0; offset < pcls; ++offset) {
		Walrus_MapItem *item = &map->items.heap[(hash + offset) % map->size];
		
		if (strcmp(item->key, key) == 0)
			return item;
	}

	return NULL;
}

void Walrus_MapRemove(Walrus_Map *map, const char *key) {

	Walrus_MapItem *remove_item = Walrus_MapProbe(map, key);
	if (!remove_item){
		Walrus_RaiseError(WALRUS_ERR_MISSING_KEY);
		Walrus_ErrorPushParam(0, strdup(key), true);
		return;
	}
	int position = ((char *)remove_item - (char *)map->items.heap) / sizeof(Walrus_MapItem);

	remove_item->key = NULL;
	remove_item->pcl = 0;

	for(int i = 1; i < map->size; i++) {
		Walrus_MapItem *current = &map->items.heap[(position + i) % map->size];

		if(current->pcl <= 0 || !current->key) break;
		
		int current_pcls = map->items.heap[(position + i - 1) % map->size].pcl_count;

		map->items.heap[(position + i - 1) % map->size] = *current;
		map->items.heap[(position + i - 1) % map->size].pcl_count = current_pcls;
		current->key = NULL;
		current->pcl = 0;
	}
}

void Walrus_MapFree(Walrus_Map *map) {
	if(!map) return;

	for(size_t i = 0; i < map->size; ++i) {
		if(map->items.heap[i].key) {
			free(map->items.heap[i].key);
			Walrus_FreeObject(map->items.heap[i].object);
		}
		map->items.heap[i].key = NULL;
		map->items.heap[i].object = NULL;
	}

	if(map->size > PREALLOCATED_MAP_BUFFER) 
		free(map->items.heap);
	map->items.heap = map->items.stack;
	map->size = PREALLOCATED_MAP_BUFFER;
}

void _DumpMap_(Walrus_Map *map) {
	printf("Hash Map Dump, allocated = %ld, used = %ld\n", map->size, map->occopied);
	for(int i = 0; i < map->size; i++)
	{
		const Walrus_MapItem *current = map->items.heap + i;

		if(!current->key) {
			printf("[%d] UNUSED\n", i);
		}else {
			printf("[%d] %s (%zu) = %p, pcl = %d [pcls = %d]\n", 
				i, current->key, Walrus_HashString(current->key, map->size),
				current->object, current->pcl, current->pcl_count);
		}
	}
}