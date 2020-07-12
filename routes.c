#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "libctemplate/ctemplate.h"
#include "lib.h"

void index_page(Response *resp)
{
    resp->TMPL_file = strdup("templates/index.html");
    resp->TMPL_mainlist = TMPL_add_var(resp->TMPL_mainlist, "hello", "I'm the index page.", 0);
}

void hello_page(Response *resp)
{
    resp->TMPL_file = strdup("templates/index.html");
    resp->TMPL_mainlist = TMPL_add_var(resp->TMPL_mainlist, "hello", "Hello world!", 0);
}




void map_route(const Request *req, Response *resp)
{
    if (strcmp(req->route, "/") == 0)
        index_page(resp);
    else if (strcmp(req->route, "/hello") == 0)
        hello_page(resp);
    else
    {
        resp->status = HTTP_NOTFOUND;
        add_error(resp, NO_ROUTE);
    }
}
