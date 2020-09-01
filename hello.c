#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>

#include "libctemplate/ctemplate.h"
#include "server/server.h"

int indexPage(Response *resp, Request *req)
{
    char *name = getRequestPostField(req, "name");
    // char *age = getRequestPostField(req, "age");

    resp->TMPL_file = setPath("index.html");
    resp->TMPL_mainlist = TMPL_add_var(resp->TMPL_mainlist, 
            "hello", "I'm the index page.",
            "name", name, 0);

    return 0;
}

int helloPage(Response *resp, Request *req)
{
    resp->TMPL_file = setPath("index.html");

    TMPL_loop *loop = 0;
    TMPL_varlist *vl;

    char *n = getRouteParam(req, 1);
    int num = atoi(n ? n : "3");
    free(n);

    for (int i = num; i > 0; i--)
    {
        vl = TMPL_add_var(0, "hello", "Hello world!",0);
        loop = TMPL_add_varlist(loop, vl);
    }
    resp->TMPL_mainlist = TMPL_add_loop(resp->TMPL_mainlist, "hello_loop", loop);

    return 0;
}
