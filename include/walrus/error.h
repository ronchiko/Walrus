
#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
	WALRUS_ERR_INVALID_FILE,
	WALRUS_ERR_INVALID_STREAM_TYPE,
	WALRUS_ERR_STRING_NULL,
	WALRUS_ERR_INVALID_CHARACTER,
	WALRUS_ERR_UNCLOSED_STRING,
	WALRUS_ERR_UNEXPECTED_TOKEN,
	WALRUS_ERR_INVALID_OPERATOR,
	WALRUS_ERR_INVALID_TOKEN,
	WALRUS_ERR_INVALID_CONSTANT,
	WALRUS_ERR_DUPLICATE_KEY,
	WALRUS_ERR_MISSING_KEY,
	WALRUS_ERR_INVALID_TYPE,
	WALRUS_ERR_TYPE_TOO_BIG
} Walrus_ErrorCode;

#define INT_2_VOIP(x) ((void *)(uint64_t)(x))
#define FLT_2_VOIP(x) ((void *)(double)(x))

void Walrus_RaiseError(Walrus_ErrorCode err);
void Walrus_ErrorPushParam(int paramIndex, void *value, bool needToFree);

void Walrus_ClearError(void);

bool Walrus_HasError(void);
const char *Walrus_GetError(void);