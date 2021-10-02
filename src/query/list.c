
#include <_query.h>

bool Walrus_QueryBranchOnList(Walrus_Object *root, Walrus_Stream *stream, Walrus_QueryContext *result) {
	Walrus_Node *node = root->list.head;
	for(size_t i = 0; node && i < root->list.elements; ++i) {
		if (!node->object) continue;

		Walrus_Stream branchStream;
		Walrus_StreamCopyState(&branchStream, stream);

		if(!Walrus_PerformQuery(node->object, &branchStream, result)) return false;
		node = node->next;
	}

	return true;
}

bool Walrus_QueryGetListItem(Walrus_Object *root, Walrus_Stream *stream, Walrus_QueryContext *result) {

	char token[QUERY_ITEM_LENGTH];
	Walrus_TokenType type = Walrus_GetAny(stream, token, QUERY_ITEM_LENGTH);

	switch(type) {
		case WALRUS_TOKEN_EOF: return Walrus_PushQueryResult(result, root);
		case WALRUS_TOKEN_CHARACTER:
			if(*token == '*') return Walrus_QueryBranchOnList(root, stream, result);
			break;

		case WALRUS_TOKEN_NUMERIC: {
			union {
				int index;
				void *generic;
			} _out_value;

			Walrus_Type indexType = Walrus_ConvertNumeric(token, &_out_value);
			if (indexType != WALRUS_INTEGER) {
				Walrus_RaiseError(WALRUS_ERR_INVALID_TOKEN);
				Walrus_ErrorPushParam(0, "integer", false);	
				return false;
			}

			Walrus_Node *child = Walrus_ListIndexedGet(&root->list, _out_value.index);
			if(!child) return true; // Out of range errors are ignored
			
			return Walrus_PerformQuery(child->object, stream, result);
		} break;

		default: break;
	}

	Walrus_RaiseError(WALRUS_ERR_INVALID_TOKEN);
	Walrus_ErrorPushParam(0, "index or '*'", false);
	return true;
}