#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libctemplate/ctemplate.h"
#include "lib.h"

int max ( int a, int b ) { return a > b ? a : b; }
int min ( int a, int b ) { return a < b ? a : b; }

void check_file(Response *resp)
{
    const char *fname = resp->TMPL_file;
    if (!fname)
    {
        add_error(resp, NO_FILE);
        resp->status = HTTP_NOTFOUND;
    }
    else if (access(fname, F_OK) == -1)
    {
        add_error(resp, NO_FILE);
        resp->status = HTTP_NOTFOUND;
    }
    else if (access(fname, R_OK) == -1)
    {
        add_error(resp, FORBIDDEN);
        resp->status = HTTP_FORBIDDEN;
    }
}
