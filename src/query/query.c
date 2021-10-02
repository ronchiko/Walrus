
#include <string.h>
#include <stdlib.h>

#include <walrus.h>
#include <_query.h>





bool Walrus_PerformQuery(Walrus_Object *root, Walrus_Stream *stream, Walrus_QueryContext *result) {
	
	char token[2];
	Walrus_TokenType type = Walrus_GetAny(stream, token, 2);
	
	switch(type) {
		case WALRUS_TOKEN_CHARACTER:
			switch(token[0]) {
				case '.': return Walrus_QueryGetProperty(root, stream, result);
				case '#': return Walrus_QueryGetListItem(root, stream, result);
				case ':': return Walrus_QueryTypeMatch(root, stream, result);

				default:
					Walrus_RaiseError(WALRUS_ERR_INVALID_OPERATOR);
					Walrus_ErrorPushParam(0, strdup(token), true);
					return false;
			};
			break;
		case WALRUS_TOKEN_EOF:
			return Walrus_PushQueryResult(result, root);
		default: break;
	}

	return false;
}