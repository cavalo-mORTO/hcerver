#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>

#include "libctemplate/ctemplate.h"
#include "server/server.h"

int indexPage(Response *resp, Request *req)
{

    resp->TMPL_file = setPath("index.html");

    return HTTP_OK;
}

int helloPage(Response *resp, Request *req)
{
    getUrlEncodedForm(req);
    char *name = getUrlEncodedFormField(req, "name");
    char *age = getUrlEncodedFormField(req, "age");

    printf("%s - %s\n", name, age);

    resp->TMPL_file = setPath("hello.html");
    resp->TMPL_mainlist = TMPL_add_var(resp->TMPL_mainlist, 
            "hello", "I'm the index page.",
            "name", name, 0);

    return HTTP_OK;
}
