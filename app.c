#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sqlite3.h>


#include "libctemplate/ctemplate.h"
#include "server/server.h"
#include "app.h"


int mapRoute(Request *req, Response *resp)
{
    /* if route is found return HTTP_OK */
    if (routeIs(req, "/"))
        return indexPage(resp, req);

    if (routeIs(req, "/dinosaur"))
        return dinosaurIndexPage(resp, req);

    if (routeIsRegEx(req, "/dinosaur/show/[0-9]*$"))
        return dinosaurShowPage(resp, req);

    /* no route was found */
    return HTTP_NOTFOUND;
}


char *handleRequest(char *raw_request)
{
    Response *resp = initResponse();
    if (!resp) return NULL;

    Request *req = parseRequest(raw_request);
    if (!req) 
    {
        freeResponse(resp);
        return NULL;
    }

    /* if route does not exist what is being requested is a file */
    unsigned short err = mapRoute(req, resp);
    if (err == HTTP_NOTFOUND)
        resp->TMPL_file = setPath(req->route);
    else
        addError(resp, err);

    renderContent(resp);

    size_t len = snprintf(NULL, 0, "%s\n%s", resp->header, resp->content.buf) + 1;

    /* concat response head with the body */
    char *response = calloc(len, sizeof(char));
    snprintf(response, len, "%s\n%s", resp->header, resp->content.buf);

    freeRequest(req);
    freeResponse(resp);
    return response;
}
