
#include <stdio.h>

#include <walrus.h>

void Walrus_WriteLongType(FILE *stream, Walrus_Object *object) {
	int elements = WALRUS_GET_TYPE_SIZE(object->type);
	for(int i = 0;i < elements;++i) {
		switch (object->type & WALRUS_TYPE_MASK) {
			case WALRUS_DECIMAL_BIT: fprintf(stream, "%f", object->decimal[i]); break;
			case WALRUS_INTEGER_BIT: fprintf(stream, "%d", object->integer[i]); break;
			default: break;
		}
		if (i != elements - 1) fprintf(stream, ", ");
	}
	fprintf(stream, "\n");
}

void Walrus_WriteObject(FILE *stream, Walrus_Object *object, int indent) {
	if(!object) return;

	switch (object->type)
	{
	case WALRUS_BOOL:
		fprintf(stream, "%s\n", object->boolean ? "true" : "false");
		break;
	case WALRUS_STRING:
		fprintf(stream, "'%s'\n", object->string);
		break;
	case WALRUS_LIST:
		for(Walrus_Node *node = object->list.head;node;node = node->next) {
			for(int i = 0; i < indent; i++) fprintf(stream, "\t");

			fprintf(stream, "* ");
			if((node->object->type & WALRUS_TYPE_MASK) & (WALRUS_OBJECT | WALRUS_LIST)) 
				fprintf(stream, "\n");
			
			Walrus_WriteObject(stream, node->object, indent + 1);
		}
		fprintf(stream, "\n");
		break;
	case WALRUS_OBJECT:
		for(int i = 0; i < object->map.size; i++) {
			Walrus_MapItem *item = &object->map.items.heap[i];
			
			if(item->key) {
				for(int i = 0; i < indent; i++) fprintf(stream, "\t");
				fprintf(stream, "%s", item->key);

				if ((item->object->type & WALRUS_TYPE_MASK) & (WALRUS_OBJECT | WALRUS_LIST)) {
					fprintf(stream, ":\n");
				}else fprintf(stream, " = ");

				Walrus_WriteObject(stream, item->object, indent + 1);

			}
			
		} 
		if(object->type == WALRUS_OBJECT){
			for(int i = 0; i < indent - 1; i++) fprintf(stream, "\t");
			fprintf(stream, "\n");
		}
		break;
	
	default:
		if (object->type & (WALRUS_DECIMAL_BIT | WALRUS_INTEGER_BIT)) {
			Walrus_WriteLongType(stream, object);
		}

		break;
	}
}

void Walrus_ExportObject(const char *filename, Walrus_Object *object) {
	FILE *file = fopen(filename, "w+");

	Walrus_WriteObject(file, object, 0);

	fclose(file);
}