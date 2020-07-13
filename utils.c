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


int set_mime_type(Response *resp, char *ext, char *route)
{
    if (!ext)
        return 0;

    free(resp->mime_type);

    char mime[100] = {0x0};

    if (strcmp(&ext[1], "html") == 0)
        strcpy(mime, "text/html");
    else if (strcmp(&ext[1], "css") == 0)
        strcpy(mime, "text/css");
    else if (strcmp(&ext[1], "js") == 0)
        strcpy(mime, "text/javascript");
    else if (strcmp(&ext[1], "csv") == 0)
        strcpy(mime, "text/csv");
    else if ( (strcmp(&ext[1], "jpeg")) == 0 || (strcmp(&ext[1], "jpg")) == 0 )
        strcpy(mime, "image/jpeg");
    else if (strcmp(&ext[1], "png") == 0)
        strcpy(mime, "image/png");
    else if (strcmp(&ext[1], "gif") == 0)
        strcpy(mime, "image/gif");
    else if (strcmp(&ext[1], "bmp") == 0)
        strcpy(mime, "image/bmp");
    else if (strcmp(&ext[1], "svg") == 0)
        strcpy(mime, "image/svg+xml");
    else if (strcmp(&ext[1], "mp4") == 0)
        strcpy(mime, "video/mp4");
    else
        strcpy(mime, "text/plain");
    
    resp->mime_type = strdup(mime);
    resp->TMPL_file = strdup(route);

    return 1;
}
