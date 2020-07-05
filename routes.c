#include <stdlib.h>
#include <string.h>

#include "lib.h"

void index_page(Response *resp)
{
    append_arg(resp, "hello", "Edit routes.c, I'm in index_page function.");
    resp->file = strdup("static/index.html");
}

void hello_page(Response *resp)
{
    append_arg(resp, "hello", "Hello world!");
    resp->file = strdup("static/index.html");
}




void map_route(Request *req, Response *resp)
{
    if (strcmp(req->route, "/") == 0)
        index_page(resp);
    else if (strcmp(req->route, "/hello") == 0)
        hello_page(resp);
    else
        append_arg(resp, "error", "Route not found!");
}
