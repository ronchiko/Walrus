
#include <string.h>
#include <stdlib.h>

#include <walrus/parsing.h>

// A line can have up to 128 * NAME_BUFFER_SIZE
#define MAX_LINE_SIZE 	NAME_BUFFER_SIZE << 7
#define MAX_TOKEN_SIZE	NAME_BUFFER_SIZE

typedef struct {
	Walrus_Stream *stream;
	int linenum;
} Walrus_Context;

static Walrus_Type Walrus_ParseProperty(Walrus_Property *prop, Walrus_Stream *line, Walrus_Stream *stream, 
								 		int linenum, int indent, Walrus_ErrorBuffer *buf);


static bool Walrus_ExtendObject(Walrus_Object *base, Walrus_Object *extension) {	
	if (!base || !extension) {
		Walrus_RaiseError(WALRUS_ERR_INVALID_TYPE);
		return false;	
	}

	Walrus_Type left = base->type, right = extension->type;
	if((left & WALRUS_TYPE_MASK) != (right & WALRUS_TYPE_MASK)) {	
		Walrus_RaiseError(WALRUS_ERR_INVALID_TYPE);
		return false;
	}
	
	char outSize = WALRUS_GET_TYPE_SIZE(left) + WALRUS_GET_TYPE_SIZE(right);
	if (outSize > MAX_COMPLEX_SIZE) {
		Walrus_RaiseError(WALRUS_ERR_TYPE_TOO_BIG);
		return false;
	}

	Walrus_Type outType = (Walrus_Type)(outSize | (left & WALRUS_TYPE_MASK));
	int elementSize		= 0;
	switch(left & WALRUS_TYPE_MASK) {
		case WALRUS_DECIMAL_BIT: elementSize = sizeof(float); 	break;
		case WALRUS_INTEGER_BIT: elementSize = sizeof(int);		break;
		default: 
			Walrus_RaiseError(WALRUS_ERR_INVALID_TYPE);
			return false;
	}

	memcpy(base->integer + WALRUS_GET_TYPE_SIZE(left), 
		   extension->integer, 
		   WALRUS_GET_TYPE_SIZE(right) * elementSize);
	base->type = outType;
	return true;
}

static Walrus_Object *Walrus_ParseSimple(Walrus_Stream *line, int linenum) {
	Walrus_Object *obj = NULL;

	char token[MAX_TOKEN_SIZE];
	Walrus_TokenType type = Walrus_GetAny(line, token, MAX_TOKEN_SIZE);
	if (type != WALRUS_TOKEN_STRING && type != WALRUS_TOKEN_NUMERIC && type != WALRUS_TOKEN_WORD)
	{
		Walrus_RaiseError(WALRUS_ERR_INVALID_TOKEN);
		Walrus_ErrorPushParam(0, "string, numeric or constant", false);
		goto cleanup;
	}

	bool isTrue;
	obj = malloc(sizeof(Walrus_Object));
	switch(type) {
		case WALRUS_TOKEN_NUMERIC:
			obj->type = Walrus_ConvertNumeric(token, &obj->integer);
			if(obj->type != WALRUS_DECIMAL && obj->type != WALRUS_INTEGER) {
				free(obj);
				return NULL;
			}
			break;
		case WALRUS_TOKEN_STRING:
			obj->type = WALRUS_STRING;
			obj->string = strndup(token, MAX_TOKEN_SIZE);
			break;
		case WALRUS_TOKEN_WORD:
			isTrue = !strcmp(token, "true");
			if(isTrue || !strcmp(token, "false"))
			{
				obj->type = WALRUS_BOOL;
				obj->boolean = isTrue;
				break;
			}

		default:
			Walrus_RaiseError(WALRUS_ERR_INVALID_TOKEN);
			Walrus_ErrorPushParam(0, "string, numeric or constant", false);
			Walrus_FreeObject(obj);
			return NULL;
	}

cleanup:
	return obj;
}

static Walrus_Object *Walrus_ParseComplex(Walrus_Stream *line, int linenum) {

	Walrus_Object *base = Walrus_ParseSimple(line, linenum), *extend;
	if (!base)
		return NULL;

	char operator[2];
	Walrus_TokenType operatorType = Walrus_GetAny(line, operator, 2);
	if (operatorType == WALRUS_TOKEN_EOF) return base;
	if (operatorType != WALRUS_TOKEN_CHARACTER) {
		Walrus_RaiseError(WALRUS_ERR_INVALID_TOKEN);
		Walrus_ErrorPushParam(0, "operator", false);
		Walrus_FreeObject(base);
		return NULL;
	}
	
	switch (*operator)
	{
	case 0:
		break;
	
	case ',':
		extend = Walrus_ParseComplex(line, linenum);
		if(!base || !extend || !Walrus_ExtendObject(base, extend)){
			Walrus_FreeObject(base);
			Walrus_FreeObject(extend);
			Walrus_RaiseError(WALRUS_ERR_INVALID_OPERATOR);
			Walrus_ErrorPushParam(0, strndup(operator, 2), true);
			Walrus_ErrorPushParam(1, INT_2_VOIP(linenum), false);
			return NULL;
		}
		Walrus_FreeObject(extend);
		break;
	default:
		Walrus_RaiseError(WALRUS_ERR_INVALID_OPERATOR);
		Walrus_ErrorPushParam(0, strndup(operator, 2), true);
		Walrus_ErrorPushParam(1, INT_2_VOIP(linenum), false);
		Walrus_FreeObject(base);
		return NULL;
	}

	return base;
}

static Walrus_Object *Walrus_ParseObject(Walrus_Stream *stream, int linenum, int parent_indent, 
										 Walrus_ErrorBuffer *errors) {
	
	static char line[MAX_LINE_SIZE];
	
	Walrus_Object *object = malloc(sizeof(Walrus_Object));
	object->type = WALRUS_ERROR;
	Walrus_MapInit(&object->map);

	Walrus_Property property;
	while(1) {
		if (Walrus_HasError()) {
			if (errors) {
				errors->errors = realloc(errors->errors, ++errors->size * sizeof(char *));
				errors->errors[errors->size - 1] = strdup(Walrus_GetError());
			}else break;
		}

		
		int indentation;
		char c = line[0], stream_buffer[MAX_LINE_SIZE];
		Walrus_Stream lineStream;
		do {
			if (!c) {
				if(!Walrus_GetLine(stream, line, MAX_LINE_SIZE)) goto cleanup;
			}

			strncpy(stream_buffer, line, MAX_LINE_SIZE);
			if(!Walrus_StreamFromSource(stream_buffer, &lineStream)) goto cleanup;

			indentation = Walrus_GetIndent(&lineStream);
			Walrus_ClearWhite(&lineStream);
			c = (*lineStream.current)(&lineStream);
		} while (!c);
		
		if (indentation != parent_indent + 1)
			break;
		
		//printf("[%d %d] %s\n", parent_indent, indentation, line);
		line[0] = 0;	
		Walrus_Type type = Walrus_ParseProperty(&property, &lineStream, stream, linenum, indentation, errors);
		if (type && (!object->type || (type == object->type))) {
			object->type = type;
			switch(type) {
				case WALRUS_OBJECT: Walrus_MapInsert(&object->map, property.name, property.object); break;
				case WALRUS_LIST: 	Walrus_ListInsert(&object->list, property.object); break;
				default: break;
			}
		}
	}

	// Its an empty object
	if (!object->type) object->type = WALRUS_OBJECT;
cleanup:
	return object;
}

static Walrus_Object *Walrus_ParseListElement(Walrus_Stream *line, Walrus_Stream *stream, 
								 			  int linenum, int parent_indent, 
											  Walrus_ErrorBuffer *errors) {
	Walrus_ClearWhite(line);
	if ((*line->current)(line) <= 0) {
		return Walrus_ParseObject(stream, linenum + 1, parent_indent, errors);
	}
	return Walrus_ParseComplex(line, linenum);

	Walrus_RaiseError(WALRUS_ERR_INVALID_TOKEN);
	Walrus_ErrorPushParam(0, "string, number, object or list", false);
	return NULL;
	
}

static Walrus_Type Walrus_ParseProperty(Walrus_Property *prop, Walrus_Stream *line, Walrus_Stream *stream, 
								 int linenum, int parent_indent, Walrus_ErrorBuffer *errors) {
	
	Walrus_TokenType opener = Walrus_GetAny(line, prop->name, MAX_TOKEN_SIZE);
	if(!(opener & (WALRUS_TOKEN_CHARACTER | WALRUS_TOKEN_WORD)))
		goto cleanup;

	if ((opener & WALRUS_TOKEN_CHARACTER) && prop->name[0] == '*') {
		prop->object = Walrus_ParseListElement(line, stream, linenum, parent_indent, errors);
		return prop->object ? WALRUS_LIST : WALRUS_ERROR;
	}

	char operator[2] = { 0 };
	Walrus_TokenType type = Walrus_GetAny(line, operator, 2);
	if (!(type & WALRUS_TOKEN_CHARACTER)) {
		Walrus_RaiseError(WALRUS_ERR_INVALID_TOKEN);
		Walrus_ErrorPushParam(0, "operator", false);
		goto cleanup;
	}

	if (operator[0] == ':') {
		return (bool)(prop->object = Walrus_ParseObject(stream, linenum + 1, parent_indent, errors)) ? 
					WALRUS_OBJECT : WALRUS_ERROR;
	}
	if (operator[0] == '=') {
		return (bool)(prop->object = Walrus_ParseComplex(line, linenum)) ? 
					WALRUS_OBJECT : WALRUS_ERROR;
	}

	Walrus_RaiseError(WALRUS_ERR_INVALID_OPERATOR);
	Walrus_ErrorPushParam(0, strndup(operator, 2), true);
	Walrus_ErrorPushParam(1, INT_2_VOIP(linenum), false);
cleanup:
	return WALRUS_ERROR;
}

Walrus_Object *Walrus_OpenFile(const char *path, Walrus_ErrorBuffer *errors) {
	Walrus_Stream *stream = Walrus_StreamFromFile(path);

	if (!stream)
		return NULL;

	Walrus_FreeErrorBuffer(errors);
	Walrus_Object *object = Walrus_ParseObject(stream, 0, -1, errors);
	Walrus_FreeStream(stream);
	
	return object;
}

void Walrus_FreeObject(Walrus_Object *object) {

	if(!object) return;

	switch (object->type)
	{
	case WALRUS_STRING:
		free(object->string);
		break;

	case WALRUS_INTEGER:
	case WALRUS_DECIMAL:
		break;
	case WALRUS_LIST:
		Walrus_ListFree(&object->list);
		break;
	case WALRUS_OBJECT:
		Walrus_MapFree(&object->map);
		break;
	default:
		break;
	}

	free(object);
}

void Walrus_FreeErrorBuffer(Walrus_ErrorBuffer *buf) {
	if( !buf ) return;

	for(size_t i = 0; i < buf->size; ++i)
		free(buf->errors[i]);

	free(buf->errors);
	buf->errors = (void *)(buf->size = 0);
}