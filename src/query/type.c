
#include <string.h>

#include <walrus.h>
#include <_query.h>

#define INT_TYPE_STRING_HASH 		0x000165DB
#define FLOAT_TYPE_STRING_HASH		0x0476787A
#define DECIMAL_TYPE_STRING_HASH	0xE593BD0A3
#define OBJECT_TYPE_STRING_HASH 	0x8BFEB00F
#define STRING_TYPE_STRING_HASH 	0x91A7E1F3
#define BOOL_TYPE_STRING_HASH 		0x0025F210
#define LIST_TYPE_STRING_HASH 		0x00299788


Walrus_Type Walrus_GetTypeEnum(const char *word) {
	size_t hash = Walrus_HashString(word, UINT64_MAX);
	switch(hash) {
		case INT_TYPE_STRING_HASH: 		return WALRUS_INTEGER;
		case FLOAT_TYPE_STRING_HASH:
		case DECIMAL_TYPE_STRING_HASH: 	return WALRUS_DECIMAL;
		case OBJECT_TYPE_STRING_HASH: 	return WALRUS_OBJECT;
		case LIST_TYPE_STRING_HASH:		return WALRUS_LIST;
		case BOOL_TYPE_STRING_HASH:		return WALRUS_BOOL;
		case STRING_TYPE_STRING_HASH:	return WALRUS_STRING;

		default: {
			union {
				int value;
				void *generic;
			} size = { .value = 0 };
			bool isInt = !strncmp(word, "int", 3); 
			if(isInt || !strncmp(word, "vec", 3)) {
				char numberBuffer[4];
				strncpy(numberBuffer, word + 3, 4);
				Walrus_Type numberType = Walrus_ConvertNumeric(numberBuffer, &size);
				if (numberType != WALRUS_INTEGER)
					goto _error;

				return (isInt ? WALRUS_INTEGER_TYPE(size.value) : WALRUS_DECIMAL_TYPE(size.value));
			}
		} break;
	}

_error:
	Walrus_RaiseError(WALRUS_ERR_INVALID_TYPE);
	return WALRUS_ERROR;
}

bool Walrus_QueryTypeMatch(Walrus_Object *root, Walrus_Stream *stream, Walrus_QueryContext *result) {
	
	char token[QUERY_ITEM_LENGTH];
	Walrus_Type type = Walrus_GetAny(stream, token, QUERY_ITEM_LENGTH);

	if (type & WALRUS_TOKEN_WORD) {
		Walrus_Type typeCast = Walrus_GetTypeEnum(token);
		if (!typeCast)
			goto error_;
		
		if (root->type == typeCast) return Walrus_PerformQuery(root, stream, result);
		return true;
	}

error_:
	Walrus_RaiseError(WALRUS_ERR_INVALID_TOKEN);
	Walrus_ErrorPushParam(0, "type name", false);
	return false;
}