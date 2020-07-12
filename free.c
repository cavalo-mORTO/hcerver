#include <stdlib.h>
#include <stdio.h>

#include "libctemplate/ctemplate.h"
#include "lib.h"

void free_dict(Dict *d) {
    if (d) {
        free(d->key);
        free(d->value);
        free_dict(d->next);
        free(d);
    }
}

void free_error(Error *e) {
    if (e) {
        free_error(e->next);
        free(e->msg);
        free(e);
    }
}

void free_request(Request *req) {
    free(req->route);
    free(req->version);
    free_dict(req->queries);
    free_dict(req->headers);
    free(req->body);
    free(req);
}

void free_response(Response *resp) {
    free_error(resp->errors);
    free(resp->header);
    free(resp->content);
    TMPL_free_varlist(resp->TMPL_mainlist);
    free(resp->TMPL_file);
    free(resp->repr);
    free(resp);
}
