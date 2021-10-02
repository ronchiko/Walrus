#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <walrus/error.h>
#include <walrus/stream.h>

#include <walrus/parsing.h>

#include <walrus/list.h>
#include <walrus/map.h>

#include <walrus/query.h>

void Walrus_WriteObject(FILE *stream, Walrus_Object *object, int indent);
#define Walrus_PrintObjectIndent(obj, i) Walrus_WriteObject(stdout, (obj), (i))
#define Walrus_PrintObject(obj) Walrus_PrintObjectIndent((obj), 0)

void Walrus_ExportObject(const char *name, Walrus_Object *object);

#define WALRUS_VERSION "0.1.0"