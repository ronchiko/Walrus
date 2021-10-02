#pragma once

#include <stdbool.h>

#include <walrus/error.h>
#include <walrus/stream.h>

#include <walrus/list.h>
#include <walrus/map.h>

#if MAX_COMPLEX_SIZE > 0xF
#warning "MAX_COMPLEX_SIZE cannot exceed 15, defaulting to 15" 
#undef MAX_COMPLEX_SIZE
#define MAX_COMPLEX_SIZE 0xF
#endif

#ifndef MAX_COMPLEX_SIZE
#define MAX_COMPLEX_SIZE 6
#endif

#ifndef NAME_BUFFER_SIZE
#define NAME_BUFFER_SIZE		64
#endif

#define WALRUS_BOOLEAN_BIT		0x010
#define WALRUS_INTEGER_BIT		0x020
#define WALRUS_DECIMAL_BIT		0x040
#define WALRUS_STRING_BIT		0x080
#define WALRUS_LIST_BIT			0x100
#define WALRUS_OBJECT_BIT		0x200

#define WALRUS_FREE_BIT			0x1000

#define WALRUS_TYPE_MASK		0x0FF0
#define WALRUS_SIZE_MASK		0x000F

#define WALRUS_DECIMAL_TYPE(n)	(WALRUS_DECIMAL_BIT | (n))
#define WALRUS_INTEGER_TYPE(n)	(WALRUS_INTEGER_BIT | (n))

#define WALRUS_GET_TYPE_SIZE(t)	((t) & WALRUS_SIZE_MASK)

typedef enum {
	WALRUS_ERROR 	= 0,

	WALRUS_OBJECT 	= WALRUS_OBJECT_BIT | WALRUS_FREE_BIT,
	WALRUS_INTEGER 	= WALRUS_INTEGER_TYPE(1),
	WALRUS_DECIMAL	= WALRUS_DECIMAL_TYPE(1),
	WALRUS_STRING 	= WALRUS_STRING_BIT | WALRUS_FREE_BIT,
	WALRUS_LIST		= WALRUS_LIST_BIT | WALRUS_FREE_BIT,

	WALRUS_VECTOR2	= WALRUS_DECIMAL_TYPE(2),
	WALRUS_VECTOR3	= WALRUS_DECIMAL_TYPE(3),
	WALRUS_COLOR	= WALRUS_DECIMAL_TYPE(4),
	WALRUS_INT2		= WALRUS_INTEGER_TYPE(2),
	WALRUS_INT3		= WALRUS_INTEGER_TYPE(3),
	WALRUS_INT4		= WALRUS_INTEGER_TYPE(4),

	WALRUS_BOOL		= WALRUS_BOOLEAN_BIT,
} Walrus_Type;

typedef struct {
	char **errors;
	size_t size;
} Walrus_ErrorBuffer;

#define WALRUS_EMPTY_ERROR_BUFFER {(void *)0, 0}

struct _Wal_Obj {
	Walrus_Type type;
	union {
		int integer[16];
		float decimal[16];
		char *string;
		Walrus_List list, complex;
		Walrus_Map	map;
		bool boolean;
	};
};

void Walrus_FreeObject(Walrus_Object *);

typedef struct __Wal_Prop {
	char name[NAME_BUFFER_SIZE];
	Walrus_Object *object;
} Walrus_Property;

/** \brief Converts a string to a float if possible */
float Walrus_ConvertToFloat(const char *buffer);
/** \brief Converts a string to an int if possible */
int Walrus_ConvertToInt(const char *buffer);
/** \brief Converts a string to an int if possible, the string is in hexadecimal */
int Walrus_ConvertHex(const char *buffer);
/** \brief Converts a string to an int if possible, the string is in binary */
int Walrus_ConvertBinary(const char *buffer);

/** 
 * \brief Detects the type of numeric provided and converts it to primitive type, returns the type of primitive it was casted to 
 * \return WALRUS_FLOAT if numeric is a float, WALRUS_INTEGER if numeric is an int or WALRUS_ERROR if something failed, Walrus_GetError to get a detailed message
 */
Walrus_Type Walrus_ConvertNumeric(const char *numeric, void *out);

Walrus_Object *Walrus_OpenFile(const char *path, Walrus_ErrorBuffer *);
void Walrus_FreeErrorBuffer(Walrus_ErrorBuffer *);
