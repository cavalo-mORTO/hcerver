#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "libctemplate/ctemplate.h"
#include "server/server.h"
#include "app.h"


int mapRoute(Request *req, Response *resp)
{
    /* if route is found return 0 */
    if (routeIs(req, "/"))
        return indexPage(resp, req);

    if (routeIs(req, "/hello"))
        return helloPage(resp, req);

    if (routeIsRegEx(req, "/hello/[0-9]*$"))
        return helloPage(resp, req);

    if (routeIsRegEx(req, "/hello/[0-9]*/show"))
        return indexPage(resp, req);

    /* no route was found */
    return 1;
}


char *handleRequest(char *raw_request)
{
    Response *resp = calloc(1, sizeof(Response));
    if (!resp) 
    {
        freeResponse(resp);
        return NULL;
    }
    resp->status = HTTP_OK;
    resp->errors = NULL;

    Request *req = parseRequest(raw_request);
    if (!req) 
    {
        freeResponse(resp);
        freeRequest(req);
        return NULL;
    }

    /* if route does not exist what is being requested is a file */
    if (mapRoute(req, resp) != 0)
        resp->TMPL_file = setPath(req->route);

    renderContent(resp);

    size_t len = snprintf(NULL, 0, "%s\n%s", resp->header, resp->content);

    /* concat response head with the body */
    char *response = calloc(len + 1, sizeof(char));
    sprintf(response, "%s\n%s", resp->header, resp->content);

    freeRequest(req);
    freeResponse(resp);
    return response;
}
