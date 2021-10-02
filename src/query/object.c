
#include <walrus/parsing.h>
#include <_query.h>

bool Walrus_QueryBranchOnObject(Walrus_Object *root, Walrus_Stream *stream, Walrus_QueryContext *result) {
	for(size_t i = 0; i < root->map.size; ++i) {
		register Walrus_MapItem *item = root->map.items.heap + i;
		if (!item || !item->object) continue;

		Walrus_Stream branchStream;
		Walrus_StreamCopyState(&branchStream, stream);

		if(!Walrus_PerformQuery(item->object, &branchStream, result)) return false;
	}

	return true;
}



bool Walrus_QueryGetProperty(Walrus_Object *root, Walrus_Stream *stream, Walrus_QueryContext *result) {
	if(root->type != WALRUS_OBJECT) return true;
	
	char token[QUERY_ITEM_LENGTH];
	Walrus_TokenType type = Walrus_GetAny(stream, token, QUERY_ITEM_LENGTH);

	Walrus_Object *child;
	switch(type) {
		case WALRUS_TOKEN_EOF: return Walrus_PushQueryResult(result, root);
		case WALRUS_TOKEN_CHARACTER:
			if(*token == '*') return Walrus_QueryBranchOnObject(root, stream, result);
			break;

		case WALRUS_TOKEN_WORD:
			child = Walrus_MapLookup(&root->map, token);
			if(!root || !(root->type & WALRUS_OBJECT) || !child)
				return true;
			return Walrus_PerformQuery(child, stream, result);

		default: break;
			
	}

	Walrus_RaiseError(WALRUS_ERR_INVALID_TOKEN);
	Walrus_ErrorPushParam(0, "identifier or '*'", false);
	return false;
}

