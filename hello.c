#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>

#include "libctemplate/ctemplate.h"
#include "server/server.h"

int indexPage(Response *resp, Request *req)
{
    // char *name = getRequestGetField(req, "name");
    char *name = getRequestPostField(req, "name");
    char *age = getRequestPostField(req, "age");


    resp->TMPL_file = setPath("index.html");
    resp->TMPL_mainlist = TMPL_add_var(resp->TMPL_mainlist, 
            "hello", "I'm the index page.",
            "name", name, 0);

    return HTTP_OK;
}
