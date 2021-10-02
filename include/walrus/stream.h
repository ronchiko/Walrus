#pragma once

#include <stdio.h>

#define BIT(n) (1 << (n))

typedef enum _Walrus_StreamInterp {
	WALRUS_TOKEN_EOF 		= BIT(1),
	
	WALRUS_TOKEN_WORD		= BIT(2),
	WALRUS_TOKEN_NUMERIC	= BIT(3),
	WALRUS_TOKEN_STRING		= BIT(4),
	WALRUS_TOKEN_CHARACTER	= BIT(5)
} Walrus_TokenType;

/** \brief A stream of characters, to be read from  */
typedef struct __Wal_Stream {
	enum {
		WALRUS_FILESTREAM,
		WALRUS_STRINGSTREAM,

		__WALRUS_STREAMTYPE_END
	} type;

	union {
		struct {
			const char *content;
			int index;
		} stringstream;
		struct {
			FILE *stream;
			char current;
		} filestream;
	};

	char (*advance)(struct __Wal_Stream *);
	char (*current)(struct __Wal_Stream *);
} Walrus_Stream;

/** \brief Creates a stream from a string */
bool Walrus_StreamFromSource(const char *source, Walrus_Stream *stream);
/** \brief Creates a stream from a file */
Walrus_Stream *Walrus_StreamFromFile(const char *path);

/** \brief Gets a line from a stream */
bool Walrus_GetLine(Walrus_Stream *stream, char *line, int maxSize);

int Walrus_GetIndent(Walrus_Stream *);

bool Walrus_ClearWhite(Walrus_Stream *stream);
bool Walrus_GetNumeric(Walrus_Stream *stream, char *buffer, int maxBufferSize);
bool Walrus_GetString(Walrus_Stream *stream, char *buffer, int maxBufferSize);
bool Walrus_GetWord(Walrus_Stream *stream, char *word, int maxWordSize);
bool Walrus_GetChar(Walrus_Stream *stream, char *ptr);

void Walrus_StreamCopyState(Walrus_Stream *_dst, Walrus_Stream *_src);

Walrus_TokenType Walrus_GetAny(Walrus_Stream *stream, char *buffer, int maxBufferSize);

/** \brief Frees a walrus stream */
void Walrus_FreeStream(Walrus_Stream *stream);