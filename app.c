#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "libctemplate/ctemplate.h"
#include "server/server.h"
#include "app.h"


int mapRoute(Request *req, Response *resp)
{
    int routed = 0;
    if ( routeIs(req, "/") )
    {
        indexPage(resp, req);
        routed = 1;
    }
    else if ( routeIs(req, "/hello") )
    {
        helloPage(resp);
        routed = 1;
    }
    else if ( routeIsRegEx(req, "/hello/[0-9]*$") )
    {
        helloPage(resp);
        routed = 1;
    }
    else if (routeIs(req, "/dinosaur"))
    {
        dinosaurIndexPage(resp, req);
        routed = 1;
    }
    else if (routeIsRegEx(req, "/dinosaur/show/[0-9]*$"))
    {
        dinosaurShowPage(resp, req);
        routed = 1;
    }

    return routed;
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
    if (!mapRoute(req, resp))
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
