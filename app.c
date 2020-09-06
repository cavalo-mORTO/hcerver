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
    unsigned short status;
    Request *req = parseRequest(raw_request, &status);
    if (!req) 
        return serverError(status);

    /* if route is found return HTTP_OK */
    if (routeIs(req, "/"))
        return makeResponse(req, indexPage);

    if (routeIs(req, "/hello"))
        return makeResponse(req, helloPage);

    if (routeIs(req, "/dinosaur"))
        return makeResponse(req, dinosaurIndexPage);

    if (routeIsRegex(req, "/dinosaur/show/[0-9]*$"))
        return makeResponse(req, dinosaurShowPage);

    if (routeIs(req, "/dinosaur/create"))
        return makeResponse(req, dinosaurCreatePage);

    /* no route was found
     * serve the requested file */
    return makeResponse(req, 0);
}
