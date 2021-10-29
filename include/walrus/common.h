#pragma once

#include <stdio.h>

#ifdef _MSC_VER
#define Walrus_strdup(v) 		 				_strdup((v))
#define Walrus_strncpy(d, dn, s, _max)			strncpy_s((d), (dn), (s), (_max))
#else
#define Walrus_strdup(v) 						strdup((v))
#define Walrus_strncpy(d, dn, s, _max)			strncpy((d), (s), (_max))
#endif 

FILE *Walrus_fopen(const char *path, const char *mode);
size_t Walrus_GetLineC(char **to, size_t *n, FILE *stream);