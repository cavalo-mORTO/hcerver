#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "libctemplate/ctemplate.h"
#include "server/server.h"

void indexPage(Response *resp, Request *req)
{
    char *name = getRequestUrlArg(req, "name");
    char *postName = getRequestPostArg(req, "name");

    resp->TMPL_file = setPath("index.html");
    resp->TMPL_mainlist = TMPL_add_var(resp->TMPL_mainlist, 
            "hello", "I'm the index page.",
            "name", postName, 0);
}

void helloPage(Response *resp)
{
    resp->TMPL_file = setPath("index.html");
    resp->TMPL_mainlist = TMPL_add_var(resp->TMPL_mainlist, "hello", "Hello world!", 0);
}
