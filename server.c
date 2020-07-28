#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "libctemplate/ctemplate.h"
#include "server.h"

/* routes.c */
void mapRoute(const Request *req, Response *resp);

Request *parseRequest(char *raw);
void renderContent(Response *resp);
void makeHeader(Response *resp);

void freeRequest(Request *req);
void freeResponse(Response *resp);
void freeDict(Dict_t *d);
void freeError(Error_t *e);


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

    /* if what is being requested is a file we'll serve it as is */
    if (strrchr(req->route, '.') != NULL)
        resp->TMPL_file = setPath(req->route);
    else
        mapRoute(req, resp);

    readFileOK(resp);

    renderContent(resp);
    makeHeader(resp);

    size_t len = snprintf(NULL, 0, "%s\n%s", resp->header, resp->content);

    /* concat response head with the body */
    char *response = calloc(len + 1, sizeof(char));
    sprintf(response, "%s\n%s", resp->header, resp->content);

    freeRequest(req);
    freeResponse(resp);
    return response;
}


void makeHeader(Response *resp)
{
    char mime[200] = {0x0};
    getMimeType(resp, mime);
    
    char metas[10000] = {0x0};
    getHttpMetas(resp->content, metas);

    time_t now = time(NULL);

    size_t head_len = snprintf(NULL, 0,
            "HTTP/%s %d\n"
            "Content-Type: %s\n"
            "Content-Length: %d\n"
            "Server: %s\n"
            "%s"
            "Date: %s\n",
            HTTP_VER, resp->status,
            mime,
            resp->content_lenght,
            SERVER_NAME,
            metas,
            ctime(&now));
    
    resp->header = calloc(head_len + 1, sizeof(char));
    sprintf(resp->header,
            "HTTP/%s %d\n"
            "Content-Type: %s\n"
            "Content-Length: %d\n"
            "Server: %s\n"
            "%s"
            "Date: %s\n",
            HTTP_VER, resp->status,
            mime,
            resp->content_lenght,
            SERVER_NAME,
            metas,
            ctime(&now));
}


void renderContent(Response *resp)
{
    TMPL_varlist *vl;
    TMPL_loop *loop;

    loop = 0;
    if (resp->errors)
    {
        free(resp->TMPL_file);
        resp->TMPL_file = setPath("error.html"); 
        TMPL_free_varlist(resp->TMPL_mainlist);

        for (Error_t *e = resp->errors; e; e = e->next)
        {
            char err[12] = {0x0};
            sprintf(err, "%d", e->error);
            vl = TMPL_add_var(0, "err", err, "msg", e->msg, 0);
            loop = TMPL_add_varlist(loop, vl);
        }
        resp->TMPL_mainlist = TMPL_add_loop(0, "errors", loop);

        char status[12] = {0x0};
        sprintf(status, "%d", resp->status);
        resp->TMPL_mainlist = TMPL_add_var(resp->TMPL_mainlist, "status", status, 0);
    }

    resp->content = calloc(10, sizeof(char));
    TMPL_write(resp->TMPL_file, 0, 0, resp->TMPL_mainlist, &resp->content, stderr);
    resp->content_lenght = strlen(resp->content);
}


void addError(Response *resp, unsigned short int err)
{
    Error_t *e;
    for (e = resp->errors; e; e = e->next)
    {
        if (e->error == err)
            return;
    }

    e = calloc(1, sizeof(Error_t));
    e->error = err;
    e->msg = strdup(SERVER_ERROR_MSG[err]);
    e->next = NULL;

    Error_t *p = resp->errors;
    if (p == NULL)
        resp->errors = e;
    else
    {
        while (p->next != NULL) { p = p->next; }
        p->next = e;
    }
}


void freeDict(Dict_t *d)
{
    if (d)
    {
        freeDict(d->next);
        free(d->key);
        free(d->value);
        free(d);
    }
}

void freeError(Error_t *e)
{
    if (e)
    {
        freeError(e->next);
        free(e->msg);
        free(e);
    }
}

void freeRequest(Request *req)
{
    freeDict(req->queries);
    freeDict(req->headers);
    free(req->route);
    free(req->version);
    free(req->body);
    free(req);
}

void freeResponse(Response *resp)
{
    freeError(resp->errors);
    free(resp->header);
    free(resp->content);
    TMPL_free_varlist(resp->TMPL_mainlist);
    free(resp->TMPL_file);
    free(resp);
}


Request *parseRequest(char *raw)
{
    Request *req = NULL;
    req = calloc(1, sizeof(Request));
    if (!req) {
        return NULL;
    }

    // Method
    size_t meth_len = strcspn(raw, " ");
    if (memcmp(raw, "GET", meth_len) == 0) {
        req->method = GET;
    } else if (memcmp(raw, "POST", meth_len) == 0) {
        req->method = POST;
    } else if (memcmp(raw, "HEAD", meth_len) == 0) {
        req->method = HEAD;
    } else {
        req->method = UNSUPPORTED;
    }
    raw += meth_len + 1; // move past <SP>

    // Request-URI
    size_t url_len = strcspn(raw, " ");

    // give url buffer more memory than needed so that we don't go out of bounds in the while loop
    char *url = calloc(1, url_len + 10);
    if (!url)
    {
        freeRequest(req);
        return NULL;
    }

    // copy unto it the contents of raw
    memcpy(url, raw, url_len);

    // set pointer for loop
    char *url_p = url;

    // get the route of url
    size_t route_len = min(strcspn(url_p, "?"), strcspn(url_p, " "));
    req->route = calloc(1, route_len + 1);

    if (!req->route) {
        freeRequest(req);
        return NULL;
    }
    memcpy(req->route, url_p, route_len);
    url_p += route_len + 1;

    // retrieve query
    Dict_t *query = NULL, *last_q = NULL;
    while (*url_p)
    {
        size_t arg_len = min(strcspn(url_p, "="), strcspn(url_p, " "));

        last_q = query;
        query = calloc(1, sizeof(Dict_t));
        if (!query) {
            freeRequest(req);
            return NULL;
        }

        query->key = calloc(1, arg_len + 1);
        if (!query->key) {
            freeRequest(req);
            return NULL;
        }
        memcpy(query->key, url_p, arg_len);
        url_p += arg_len + 1;

        size_t val_len = min(strcspn(url_p, "&"), strcspn(url_p, " "));
        query->value = calloc(1, val_len + 1);
        if (!query->value) {
            freeRequest(req);
            return NULL;
        }
        memcpy(query->value, url_p, val_len);
        url_p += val_len + 1;

        query->next = last_q;
    }
    req->queries = query;
    free(url);

    raw += url_len + 1; // move past <SP>

    // HTTP-Version
    size_t ver_len = strcspn(raw, "\r\n");
    req->version = calloc(1, ver_len + 1);
    if (!req->version) {
        freeRequest(req);
        return NULL;
    }
    memcpy(req->version, raw, ver_len);
    raw += ver_len + 2; // move past <CR><LF>

    Dict_t *header = NULL, *last = NULL;
    while (raw[0]!='\r' || raw[1]!='\n') {
        last = header;
        header = calloc(1, sizeof(Dict_t));
        if (!header) {
            freeRequest(req);
            return NULL;
        }

        // name
        size_t key_len = strcspn(raw, ":");
        header->key = calloc(1, key_len + 1);
        if (!header->key) {
            freeRequest(req);
            return NULL;
        }
        memcpy(header->key, raw, key_len);
        raw += key_len + 1; // move past :
        while (*raw == ' ') {
            raw++;
        }

        // value
        size_t value_len = strcspn(raw, "\r\n");
        header->value = calloc(1, value_len + 1);
        if (!header->value) {
            freeRequest(req);
            return NULL;
        }
        memcpy(header->value, raw, value_len);
        raw += value_len + 2; // move past <CR><LF>

        // next
        header->next = last;
    }
    req->headers = header;
    raw += 2; // move past <CR><LF>

    size_t body_len = strlen(raw);
    req->body = calloc(1, body_len + 1);
    if (!req->body) {
        freeRequest(req);
        return NULL;
    }
    memcpy(req->body, raw, body_len);

    return req;
}
