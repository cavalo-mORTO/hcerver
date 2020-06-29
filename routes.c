#include <stdlib.h>
#include <string.h>

#include "lib.h"

void map_route(Request *req, Response *resp)
{
    if (strcmp(req->route, "/") == 0)
        index_page(resp);
}

void index_page(Response *resp)
{
    strcpy(resp->file, "static/index.html");
}
