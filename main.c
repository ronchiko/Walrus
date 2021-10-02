
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "walrus.h"

extern void _DumpMap_ (Walrus_Map *map)  ;
extern void _DumpList_(Walrus_List *list);

struct Walrus_ShellResult {
	bool shouldQuit;
};

struct Walrus_ShellContext {
	Walrus_Object *root;
	char *file;
};

#define ERRMSG(msg) "\033[31m"msg"\033[0m\n"
#define SCSMSG(msg) "\033[32m"msg"\033[0m\n"

#define INSTRUCTION_SIZE	32

const char *PrintObjectType(Walrus_Object *object) {
	static char TYPE_GLOBAL_BUFFER[512];

	switch(object->type) {
		case WALRUS_OBJECT: return "object"; 
		case WALRUS_LIST: return "list";
		case WALRUS_INTEGER: return "int";
		case WALRUS_DECIMAL: return "decimal";
		case WALRUS_STRING: return "string";
		case WALRUS_BOOL: return "boolean";
		default:
			if ((object->type & WALRUS_TYPE_MASK) == WALRUS_INTEGER_BIT) {
				snprintf(TYPE_GLOBAL_BUFFER, 512, "int%d", object->type & WALRUS_SIZE_MASK);
				return TYPE_GLOBAL_BUFFER;
			}
			if ((object->type & WALRUS_TYPE_MASK) == WALRUS_DECIMAL_BIT) {
				snprintf(TYPE_GLOBAL_BUFFER, 512, "vec%d", object->type & WALRUS_SIZE_MASK);
				return TYPE_GLOBAL_BUFFER;
			}

			return NULL;
	}
}

void ShellPerformQuery(Walrus_Stream *line, struct Walrus_ShellContext *context) {
	char query[1024];
	Walrus_TokenType type = Walrus_GetAny(line, query, 1024);
	if (type != WALRUS_TOKEN_STRING) {
		printf(ERRMSG("Query must be given in a string format"));
		return;
	}

	// Optional parameters
	int limit = -1;
	Walrus_Object *root = context->root;
	char optional[1024];
	while((type = Walrus_GetAny(line, optional, 1024)) != WALRUS_TOKEN_EOF) {
		if ((type & WALRUS_TOKEN_WORD) && !strncmp(optional, "LIMIT", 5)) {
			type = Walrus_GetAny(line, optional, 1024);
			if(type != WALRUS_TOKEN_NUMERIC) {
				printf(ERRMSG("Query limit expects an integer"));
				return;
			}
			Walrus_Type numericType = Walrus_ConvertNumeric(optional, (void *)(uint64_t *)&limit);
			if (numericType != WALRUS_INTEGER) {
				printf(ERRMSG("Query limit expects an integer"));
				return;
			}
			continue;
		}

		printf(ERRMSG("Invalid parameter '%s' for 'GET'"), optional);
		return;
	}

	if (!root) {
		printf(ERRMSG("Root object is not specified"));
		return;
	}

	size_t resultsCount = 0;
	Walrus_Object **results = NULL;
	if(limit > 0) {
		results = malloc(sizeof(Walrus_Object *) * limit);
		resultsCount = Walrus_QueryLimit(root, query, results, limit);
	}else {
		results = Walrus_Query(root, query, &resultsCount);
	}

	printf(SCSMSG("Got %ld results"), resultsCount);
	for(int i = 0; i < resultsCount; ++i) {
		const char *typeString = PrintObjectType(results[i]);
		Walrus_Type type = results[i]->type;

		printf("\033[33m<%s>\033[0m%c", typeString, 
			(type == WALRUS_OBJECT || type == WALRUS_LIST) ? '\n' : ' ');
		Walrus_PrintObjectIndent(results[i], 1);
	}
}

void ShellLoad(const char *path, struct Walrus_ShellContext *context) {	
	Walrus_FreeObject(context->root);
	Walrus_ErrorBuffer errors = WALRUS_EMPTY_ERROR_BUFFER;
	Walrus_Object *object = Walrus_OpenFile(path, &errors);
	
	if(errors.size) {
		printf("\033[31mFailed to open File '%s'\n", path);
		for(size_t i = 0; i < errors.size; ++i)
			printf("%s\n", errors.errors[i]);
		printf("\033[0m");
		goto cleanup;
	} 
	printf("\033[33mSuccessfully opened file '%s'\033[0m\n", path);
	context->root = object;
	free(context->file);
	context->file = strdup(path);

cleanup:
	Walrus_FreeErrorBuffer(&errors);
}

struct Walrus_ShellResult HandleShellCommand(Walrus_Stream *line, struct Walrus_ShellContext *context) {
	char instruction[INSTRUCTION_SIZE];
	Walrus_TokenType type = Walrus_GetAny(line, instruction, INSTRUCTION_SIZE);
	if (type != WALRUS_TOKEN_WORD) {
		if(type != WALRUS_TOKEN_EOF)
			goto not_a_valid_command;
		return (struct Walrus_ShellResult){ .shouldQuit = false };
	}

	if (!strncmp(instruction, "EXIT", INSTRUCTION_SIZE)) 
		return (struct Walrus_ShellResult){ .shouldQuit = true };

	if (!strncmp(instruction, "GET", INSTRUCTION_SIZE)) {
		ShellPerformQuery(line, context);
		return (struct Walrus_ShellResult){ .shouldQuit = false };
	}
	if (!strncmp(instruction, "ROOT", INSTRUCTION_SIZE)) {
		Walrus_Stream query;
		Walrus_StreamFromSource("''", &query);

		ShellPerformQuery(&query, context);
		return (struct Walrus_ShellResult){ .shouldQuit = false };
	}

	if (!strncmp(instruction, "OPEN", INSTRUCTION_SIZE)) {
		char path[512];
		type = Walrus_GetAny(line, path, 512);
		if (type != WALRUS_TOKEN_STRING) {
			printf(ERRMSG("OPEN expects a string as input"));
			return (struct Walrus_ShellResult){ .shouldQuit = false }; 
		} 

		ShellLoad(path, context);
		return (struct Walrus_ShellResult){ .shouldQuit = false }; 
	}

not_a_valid_command:
	printf(ERRMSG("'%s' is not a command"), instruction);
	return (struct Walrus_ShellResult){ .shouldQuit = false };
}

int main(int argc, char *argv[]) {
	struct Walrus_ShellContext context = { NULL, NULL };
	printf("Opening Walrus Shell, Walrus version %s\n", WALRUS_VERSION);

	if (argc > 1)
		ShellLoad(argv[1], &context);


	char *lineBuffer = NULL;
	while(true) {
		free(lineBuffer);
		printf("\033[35m%s\033[0m> ", context.file);
		int size = 0; size_t n = 0;
		if((size = getline(&lineBuffer, &n, stdin)) <= 0) break;

		Walrus_Stream line;
		if(!Walrus_StreamFromSource(lineBuffer, &line)) {
			printf(ERRMSG("Walrus Error Accuered: %s"), Walrus_GetError());
			continue;
		} 

		struct Walrus_ShellResult result = HandleShellCommand(&line, &context);

		if( result.shouldQuit ) break;
	}

	free(lineBuffer);
	Walrus_ClearError();
	return 0;
}