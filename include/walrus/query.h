#pragma once

#include <walrus/parsing.h>

Walrus_Object *Walrus_QuerySingle(Walrus_Object *root, const char *query);
size_t Walrus_QueryLimit(Walrus_Object *root, const char *query, Walrus_Object *results[], size_t max_results);

/** \brief Returns an array of all the query results, make sure to free the array */
Walrus_Object **Walrus_Query(Walrus_Object *root, const char *query, size_t *results);

#define Walrus_FreeQueryArray(arr) free((arr))
