#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sqlite3.h>

#include "libctemplate/ctemplate.h"
#include "server/server.h"
#include "app.h"

char *handleRequest(char *raw_request)
{
    Request *req = parseRequest(raw_request);
    if (!req) 
        return internalServerError();

    /* if route is found return HTTP_OK */
    if (routeIs(req, "/"))
        return makeResponse(req, indexPage);

    if (routeIs(req, "/dinosaur"))
        return makeResponse(req, dinosaurIndexPage);

    if (routeIsRegEx(req, "/dinosaur/show/[0-9]*$"))
        return makeResponse(req, dinosaurShowPage);

    /* no route was found
     * serve the requested file */
    return makeResponse(req, NULL);
}
