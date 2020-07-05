#include <stdlib.h>

#include "lib.h"

void free_dict(struct Dict *d) {
    if (d) {
        free(d->key);
        free(d->value);
        free_dict(d->next);
        free(d);
    }
}

void free_request(struct Request *req) {
    free(req->route);
    free(req->version);
    free_dict(req->queries);
    free_dict(req->headers);
    free(req->body);
    free(req);
}

void free_response(struct Response *resp) {
    free(resp->content);
    free_dict(resp->args);
    free(resp->file);
    free(resp->header);
    free(resp->repr);
    free(resp);
}
