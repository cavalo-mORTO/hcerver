#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "libctemplate/ctemplate.h"
#include "server.h"

void indexPage(Response *resp, Request *req)
{
    resp->TMPL_file = setPath("index.html");
    resp->TMPL_mainlist = TMPL_add_var(resp->TMPL_mainlist, "hello", "I'm the index page.", 0);

    char *a = getRequestArg(req, "name");

    if (a)
    {
        printf("a is defined\n");
        puts(a);
    }

}

void helloPage(Response *resp)
{
    resp->TMPL_file = setPath("index");
    resp->TMPL_mainlist = TMPL_add_var(resp->TMPL_mainlist, "hello", "Hello world!", 0);
}




void mapRoute(Request *req, Response *resp)
{
    if (strcmp(req->route, "/") == 0)
        indexPage(resp, req);
    else if (strcmp(req->route, "/hello") == 0)
        helloPage(resp);
    else
    {
        resp->status = HTTP_NOTFOUND;
        addError(resp, NO_ROUTE);
    }
}
