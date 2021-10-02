
#include <stdlib.h>
#include <stdio.h>

#include "walrus/error.h"

#define WALRUS_SETF(flag, bit) 		(flag) |= (1 << (bit))
#define WALRUS_UNSETF(flag, bit) 	(flag) &= ~(1 << (bit))
#define WALRUS_GETF(flag, bit) 		((flag) & (1 << (bit)))

#define MAX_ERROR_PARAMS 	4
#define EMSG_MAX_SIZE 		256



const char *gErrorToString[] = {
	[WALRUS_ERR_INVALID_FILE] = "Failed to open file '%s'",
	[WALRUS_ERR_INVALID_STREAM_TYPE] = "Got invalid stream type %d",
	[WALRUS_ERR_STRING_NULL] = "Expected string got NULL",
	[WALRUS_ERR_INVALID_CHARACTER] = "Got unexpcted character '%c' after \\",
	[WALRUS_ERR_UNCLOSED_STRING] = "String not closed",
	[WALRUS_ERR_UNEXPECTED_TOKEN] = "Got unexpcted token at '%s'",
	[WALRUS_ERR_INVALID_OPERATOR] = "Invalid operator '%s'",
	[WALRUS_ERR_INVALID_TOKEN] = "Invalid token, expceted %s",
	[WALRUS_ERR_INVALID_CONSTANT] = "Invalid %s constant '%s', %s",
	[WALRUS_ERR_DUPLICATE_KEY] = "Duplicate key '%s' in map",
	[WALRUS_ERR_MISSING_KEY] = "Map missing key '%s'",
	[WALRUS_ERR_INVALID_TYPE] = "Invalid type",
	[WALRUS_ERR_TYPE_TOO_BIG] = "Type is too big, max size is 15 elements"
};

static bool gHasError = false;
static void *gErrorParams[MAX_ERROR_PARAMS] = {
	NULL, NULL, NULL, NULL
};
static int gErrorParamsFlags = 0; 
static Walrus_ErrorCode gErrorCode;

// Each error is formatted on this buffer
static char gErrorBuffer[EMSG_MAX_SIZE];

void Walrus_RaiseError(Walrus_ErrorCode code) {
	if(gHasError) Walrus_ClearError();

	gHasError = true;
	gErrorCode = code;
}

void Walrus_ErrorPushParam(int index, void *value, bool shouldFree) {
	// Error param out of range, ignored
	if(index < 0 || index >= MAX_ERROR_PARAMS) return;

	gErrorParams[index] = value;

	if (shouldFree) WALRUS_SETF(gErrorParamsFlags, index);
	else WALRUS_UNSETF(gErrorParamsFlags, index);
}

bool Walrus_HasError(void) {
	return gHasError;
}

const char *Walrus_GetError(void) {
	if(!gHasError) return "";

	snprintf(gErrorBuffer, EMSG_MAX_SIZE, gErrorToString[gErrorCode], 
		gErrorParams[0], gErrorParams[1], gErrorParams[2], gErrorParams[3]);
	gErrorBuffer[EMSG_MAX_SIZE - 1] = 0;

	Walrus_ClearError();
	return gErrorBuffer;
}

void Walrus_ClearError(void) {
	gHasError = false;
	for (int i = 0; i < MAX_ERROR_PARAMS; i++) {
		if (WALRUS_GETF(gErrorParamsFlags, i)) free(gErrorParams[i]);
		gErrorParams[i] = NULL;
	}
}
