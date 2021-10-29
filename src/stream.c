
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "walrus/error.h"
#include "walrus/stream.h"

static char gExtendedParsers[0xFF] = {
	['n'] = '\n',
	['f'] = '\f',
	['t'] = '\t',
	['b'] = '\b',
	['r'] = '\r',
	['0'] = '\0',
	['\\']= '\\',
	['\'']= '\'',
	['"'] = '"' 
};

static int Walrus_CharToInt(char c) {
	if('0' <= c && c <= '9') return c - '0';
	if('a' <= tolower(c) && tolower(c) <= 'f') return tolower(c) - 'a' + 0xA;

	return -1;
}

static char Walrus_FileStreamAdvance(Walrus_Stream *stream) {
	return stream->filestream.current = getc(stream->filestream.stream);
}
static char Walrus_StringStreamAdvance(Walrus_Stream *stream) {
	return stream->stringstream.content[++stream->stringstream.index];
}

static char Walrus_FileStreamCurrent(Walrus_Stream *stream) {
	return stream->filestream.current;
}
static char Walrus_StringStreamCurrent(Walrus_Stream *stream) {
	return stream->stringstream.content[stream->stringstream.index];
}

Walrus_Stream *Walrus_StreamFromFile(const char *path) {
	Walrus_Stream *stream = NULL;

	FILE *fstream = Walrus_fopen(path, "r");
	if (!fstream) {
		Walrus_RaiseError(WALRUS_ERR_INVALID_FILE);
		Walrus_ErrorPushParam(0, (void*)path, false);
		goto cleanup;
	}

	stream = malloc(sizeof(Walrus_Stream));
	stream->type = WALRUS_FILESTREAM;
	stream->filestream.stream = fstream;
	stream->advance = &Walrus_FileStreamAdvance;
	stream->current = &Walrus_FileStreamCurrent;
	(*stream->advance)(stream);

cleanup:
	return stream;
}

bool Walrus_StreamFromSource(const char *source, Walrus_Stream *stream) {
	if (!source) {
		Walrus_RaiseError(WALRUS_ERR_STRING_NULL);
		return false;
	}

	stream->type = WALRUS_STRINGSTREAM;
	stream->stringstream.content = source;
	stream->stringstream.index = 0;
	stream->advance = &Walrus_StringStreamAdvance;
	stream->current = &Walrus_StringStreamCurrent;
	return true;
}

bool Walrus_GetChar(Walrus_Stream *stream, char *ptr) {
	if((*stream->current)(stream) <= 0) return false;
	
	char n = (*stream->current)(stream);
	if(ptr) *ptr = n;
	(*stream->advance)(stream);

	return true;
}

bool Walrus_GetLine(Walrus_Stream *stream, char *line, int maxSize) {
	
	char current = (*stream->current)(stream);
	if(current <= 0) return false;

	int used = 0;
	
	while(current != '\n' && current > '\0' && current != '\r' && used < maxSize - 1) {		
		line[used++] = current;
		current = (*stream->advance)(stream);
	}

	if(current == '\r') {
		current = (*stream->advance)(stream);
		if(current != '\n') {
			switch (stream->type)
			{
			case WALRUS_FILESTREAM:
				stream->filestream.current = '\r';
				ungetc(current, stream->filestream.stream);
				break;
			case WALRUS_STRINGSTREAM:
				stream->stringstream.index--;
				break;
			
			default:
				Walrus_RaiseError(WALRUS_ERR_INVALID_STREAM_TYPE);
				Walrus_ErrorPushParam(0, (void *)stream->type, false);
				break;
			}
		}
	}
	
	if(current != 0) (*stream->advance)(stream);

	line[used] = 0;
	return true;
}

bool Walrus_ClearWhite(Walrus_Stream *stream) {
	char c = (*stream->current)(stream);
	if(c <= 0) return false;
	
	while(isspace(c))
		c = (*stream->advance)(stream);

	return true;
}

static bool Walrus_HandleBeginningDot(Walrus_Stream *stream, char *buffer, int size) {
	char current = (*stream->advance)(stream);

	if(!isdigit(current)) {
		buffer[0] = '.';
		buffer[1] = 0;
		return true;
	}

	return Walrus_GetNumeric(stream, buffer, size);
}

bool Walrus_GetNumeric(Walrus_Stream *stream, char *buffer, int size) {
	if((*stream->current)(stream) <= 0) return false;
	Walrus_ClearWhite(stream);

	int i = 0;
	char c = (*stream->current)(stream);
	while((isdigit(c) || c == 'x' || c == '.' || ('a' <= tolower(c) && tolower(c) <= 'f')) 
		&& i < size - 1) { 	
			
		buffer[i++] = c;
		c = (*stream->advance)(stream);
	}

	buffer[i] = 0;
	return true;
	
}

bool Walrus_GetWord(Walrus_Stream *stream, char *buffer, int maxSize) {
	
	char c = (*stream->current)(stream);
	if(c <= 0) return false;
	Walrus_ClearWhite(stream);

	int i = 0;
	while((isalnum(c) || c == '_') && i < maxSize - 1) { 		
		buffer[i++] = c;
		c = (*stream->advance)(stream);
	}

	buffer[i] = 0;
	return true;
}

void Walrus_StreamCopyState(Walrus_Stream *_dst, Walrus_Stream *_src) {
	memcpy(_dst, _src, sizeof(Walrus_Stream));
}

bool Walrus_GetString(Walrus_Stream *stream, char *buffer, int maxSize) {
	
	char c = (*stream->current)(stream);
	if(c <= 0) return false;
	Walrus_ClearWhite(stream);

	int i = 0;
	if(c != '\'' && c != '\"') goto cleanup;

	char quotes = c;
	c = (*stream->advance)(stream);
	while(c != quotes && i < maxSize - 1) {
		if(c == 0) {
			Walrus_RaiseError(WALRUS_ERR_UNCLOSED_STRING);		
			goto cleanup;
		}
		
		if (c == '\\') {
			char replaceWith = 0;
			c = (*stream->advance)(stream);
			switch (c)
			{
			default:
				replaceWith = gExtendedParsers[(int)c];
				if(c != '0' && !replaceWith){
					Walrus_RaiseError(WALRUS_ERR_INVALID_CHARACTER);
					Walrus_ErrorPushParam(0, INT_2_VOIP(c), false);
					goto cleanup;
				}
				break;
			case 'x': {
				char hex[2] = {(*stream->advance)(stream), (*stream->advance)(stream)};
				int left 	= Walrus_CharToInt(hex[0]), 
					right 	= Walrus_CharToInt(hex[1]);
				
				if (left < 0 || right < 0)  {
					Walrus_RaiseError(WALRUS_ERR_INVALID_CHARACTER);
					Walrus_ErrorPushParam(0, INT_2_VOIP(c), false);
					goto cleanup;
				}
	
				replaceWith = right | (left << 4);
				} break;
			}
			c = replaceWith;
		}

		buffer[i++] = c;
		c = (*stream->advance)(stream);
	}
	c = (*stream->advance)(stream);

cleanup:
	buffer[i] = 0;
	return true;
}

enum _Walrus_StreamInterp Walrus_GetAny(Walrus_Stream *stream, char *buffer, int maxBufferSize) {
	
	Walrus_ClearWhite(stream);

	char current = (*stream->current)(stream);
	if(isalpha(current)) current = 'a';
	if(isdigit(current)) current = '0';

	static struct { 
		bool (*method)(Walrus_Stream *, char *, int);
		enum _Walrus_StreamInterp interp;
		} mParsingMethods[0xFF] = {
			['0'] = 		{ &Walrus_GetNumeric, WALRUS_TOKEN_NUMERIC },
			['a'] = 		{ &Walrus_GetWord, WALRUS_TOKEN_WORD },
			['_'] = 		{ &Walrus_GetWord, WALRUS_TOKEN_WORD },
			['\''] = 		{ &Walrus_GetString, WALRUS_TOKEN_STRING},
			['"'] = 		{ &Walrus_GetString, WALRUS_TOKEN_STRING},

			['.'] = 		{ &Walrus_HandleBeginningDot, WALRUS_TOKEN_NUMERIC }
	};

	bool result = false;
	bool (*parser)(Walrus_Stream *, char *, int) = mParsingMethods[(int)current].method;
	

	enum _Walrus_StreamInterp interp = mParsingMethods[(int)current].interp;

	if (!parser) {
		interp = WALRUS_TOKEN_CHARACTER;
		result = Walrus_GetChar(stream, buffer);
		buffer[1] = 0;
	}
	else result = (*parser)(stream, buffer, maxBufferSize);

	if (current == '.' && !strcmp(buffer, ".")) interp = WALRUS_TOKEN_CHARACTER;

	return result ? interp : WALRUS_TOKEN_EOF;
}

int Walrus_GetIndent(Walrus_Stream *stream) {
	char current = (*stream->current)(stream);
	if(current < 0) return -1;

	int indent = 0;
	for(indent = 0; current == '\t'; 
		++indent, current = (*stream->advance)(stream)) {}

	return indent;
}

void Walrus_FreeStream(Walrus_Stream *stream) {
	if (!stream) return;

	switch (stream->type)
	{
	case WALRUS_FILESTREAM:
		fclose(stream->filestream.stream);
		break;
	
	case WALRUS_STRINGSTREAM: break;

	default:
		Walrus_RaiseError(WALRUS_ERR_INVALID_STREAM_TYPE);
		Walrus_ErrorPushParam(0, INT_2_VOIP(stream->type), false);
		break;
	}
	free(stream);
}

void Walrus_StreamRetreat(Walrus_Stream *stream, int amount) {
	int offset;
	switch (stream->type)
	{
	case WALRUS_FILESTREAM:
		offset = ftell(stream->filestream.stream);

		fseek(stream->filestream.stream, offset - amount, SEEK_SET);
		stream->filestream.current = getc(stream->filestream.stream);
		break;
	case WALRUS_STRINGSTREAM:
		stream->stringstream.index -= amount;
		if( stream->stringstream.index < 0) stream->stringstream.index = 0;
		break;
	default:
		Walrus_RaiseError(WALRUS_ERR_INVALID_STREAM_TYPE);
		Walrus_ErrorPushParam(0, INT_2_VOIP(stream->type), false);
		break;
	}
	
} 