#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "libctemplate/ctemplate.h"
#include "server/server.h"

int indexPage(Response *resp, Request *req)
{
    // char *name = getRequestGetField(req, "name");
    char *name = getRequestPostField(req, "name");
    char *age = getRequestPostField(req, "age");

    // printf("%s: %s\n", name, age);

    /*
    printf("\n");
    for (Dict_t *d = req->headers; d != NULL; d = d->next)
        printf("%s: %s\n", d->key, d->value);
    printf("\n");
    printf("%s\n", req->body);
    */



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
    for (int i = atoi(getRouteParam(req, 2)); i > 0; i--)
    {
        vl = TMPL_add_var(0, "hello", "Hello world!",0);
        loop = TMPL_add_varlist(loop, vl);
    }
    resp->TMPL_mainlist = TMPL_add_loop(resp->TMPL_mainlist, "hello_loop", loop);

    return 0;
}
