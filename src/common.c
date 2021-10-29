
#include <stdlib.h>

#include <walrus/common.h>

FILE *Walrus_fopen(const char *path, const char *mode) {
	FILE *stream;
#ifdef _MSC_VER
	errno_t errn = fopen_s(&stream, path, mode);
	if(errn) return NULL;
#else
	stream = fopen(path, mode);
#endif
	return stream;
}

#define GROWTH_SIZE 		1024
size_t Walrus_GetLineC(char **buf, size_t *n, FILE *stm) {
	if (!stm || !n) return 0;
	
	if (!*buf) {
		*buf = malloc(GROWTH_SIZE);
		*n = GROWTH_SIZE;
	}

	size_t szt = 0;
	int _c = fgetc(stm);
	while(_c != '\n' && _c != '\r') {
		if(szt >= GROWTH_SIZE) {
			*n += GROWTH_SIZE;
			*buf = realloc(*buf, *n);
		}

		(*buf)[szt++] = _c;
		_c = fgetc(stm);
	}

	(*buf)[szt] = 0;
	return szt;
}