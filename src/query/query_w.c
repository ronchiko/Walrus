
#include <stdlib.h>

#include <walrus/query.h>
#include <_query.h>

bool Walrus_PushQueryResult(Walrus_QueryContext *qr, Walrus_Object *result) {
	if (qr->results.limit <= qr->results.count) {
		if(!qr->results.isDynamic) return false;

		qr->results.limit = !qr->results.limit ? 5 : (size_t)(qr->results.limit * 1.5);
		qr->results.results = realloc(qr->results.results, 
											sizeof(Walrus_Object *) * qr->results.limit);
	}

	qr->results.results[qr->results.count++] = result;
	return true;
}

Walrus_Object *Walrus_QuerySingle(Walrus_Object *root, const char *query_string) {
	Walrus_Object *resultBuffer[1];
	Walrus_QueryContext query_result = {
		.results={.isDynamic=false, .count=0, .limit=1, .results=resultBuffer},
		.query=query_string
	};

	Walrus_Stream query_stream;
	if(!Walrus_StreamFromSource(query_string, &query_stream) || !root)
		return NULL;
	
	Walrus_PerformQuery(root, &query_stream, &query_result);

	return query_result.results.results[0];
}

size_t Walrus_QueryLimit(Walrus_Object *root, const char *query, Walrus_Object *results[], size_t max_results) {
	Walrus_QueryContext query_result = {
		.results={.isDynamic=false, .count=0, .limit=max_results, .results=results},
		.query=query
	};

	Walrus_Stream query_stream;
	if(!Walrus_StreamFromSource(query, &query_stream) || !root)
		return 0;
	
	Walrus_PerformQuery(root, &query_stream, &query_result);

	return query_result.results.count;
}

Walrus_Object **Walrus_Query(Walrus_Object *root, const char *query, size_t *results) {
	Walrus_QueryContext query_result = {
		.results={.isDynamic=true, .count=0, .limit=0, .results=NULL},
		.query=query
	};

	Walrus_Stream query_stream;
	if(!Walrus_StreamFromSource(query, &query_stream) || !root)
		return NULL;
	
	Walrus_PerformQuery(root, &query_stream, &query_result);

	*results = query_result.results.count;
	return query_result.results.results;
}