#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "libctemplate/ctemplate.h"
#include "server.h"

void index_page(Response *resp)
{
    resp->TMPL_file = setPath("index.html");
    resp->TMPL_mainlist = TMPL_add_var(resp->TMPL_mainlist, "hello", "I'm the index page.", 0);
}

void hello_page(Response *resp)
{
    resp->TMPL_file = setPath("forbidden.html");
    resp->TMPL_mainlist = TMPL_add_var(resp->TMPL_mainlist, "hello", "Hello world!", 0);
}




void mapRoute(const Request *req, Response *resp)
{
    if (strcmp(req->route, "/") == 0)
        index_page(resp);
    else if (strcmp(req->route, "/hello") == 0)
        hello_page(resp);
    else
    {
        resp->status = HTTP_NOTFOUND;
        addError(resp, NO_ROUTE);
    }
}
