#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libctemplate/ctemplate.h"
#include "lib.h"


char *handle_request(char *raw_request)
{
    Response *resp = calloc(1, sizeof(Response));
    if (!resp) 
    {
        free_response(resp);
        return NULL;
    }
    resp->status = HTTP_OK;
    resp->errors = NULL;

    Request *req = parse_request(raw_request);
    if (!req) 
    {
        free_response(resp);
        free_request(req);
        return NULL;
    }

    // if what is being requested is a file we'll serve it as is
    if (strrchr(req->route, '.') != NULL)
        resp->TMPL_file = strdup(&req->route[1]);
    else
        map_route(req, resp);

    check_file(resp);

    render_content(resp);
    make_header(resp);

    size_t len = resp->content_lenght + strlen(resp->header) + 1;

    // concat response head with the body
    char *response = calloc(len, sizeof(char));
    snprintf(response, len, "%s\n%s", resp->header, resp->content);

    free_request(req);
    free_response(resp);
    return response;
}


void make_header(Response *resp)
{
    char status[100] = {0x0};
    sprintf(status, "HTTP/%s %d", HTTP_VER, resp->status);

    char mime[200] = {0x0};
    get_mime_type(resp, mime);

    char content_type[220] = {0x0};
    sprintf(content_type, "Content-Type: %s", mime);

    char content_len[100] = {0x0};
    sprintf(content_len, "Content-Length: %d", resp->content_lenght);

    char *server = "Server: The WebServer with Thick Thighs";

    
    char metas[10000] = {0x0};
    get_http_metas(resp->content, metas);

    time_t now = time(NULL);
    char date[100] = {0x0};
    sprintf(date, "Date: %s", ctime(&now));

    size_t head_len = snprintf(NULL, 0, "%s\n%s\n%s\n%s\n%s%s\n", status, content_type, content_len, server, metas, date);
    
    resp->header = calloc(head_len + 1, sizeof(char));
    sprintf(resp->header, "%s\n%s\n%s\n%s\n%s%s\n", status, content_type, content_len, server, metas, date);
}


void render_content(Response *resp)
{
    TMPL_varlist *vl;
    TMPL_loop *loop;

    loop = 0;
    if (resp->errors)
    {
        free(resp->TMPL_file);
        resp->TMPL_file = strdup("templates/error.html");
        TMPL_free_varlist(resp->TMPL_mainlist);

        for (Error *e = resp->errors; e; e = e->next)
        {
            char err[100] = {0x0};
            sprintf(err, "%d", e->error);
            vl = TMPL_add_var(0, "err", err, "msg", e->msg, 0);
            loop = TMPL_add_varlist(loop, vl);
        }
        resp->TMPL_mainlist = TMPL_add_loop(0, "errors", loop);

        char status[100] = {0x0};
        sprintf(status, "%d", resp->status);
        resp->TMPL_mainlist = TMPL_add_var(resp->TMPL_mainlist, "status", status, 0);
    }

    resp->content = TMPL_write(resp->TMPL_file, 0, 0, resp->TMPL_mainlist, NULL, stderr);
    resp->content_lenght = strlen(resp->content);
}


void add_error(Response *resp, unsigned short int err)
{
    Error *e;
    for (e = resp->errors; e; e = e->next)
    {
        if (e->error == err)
            return;
    }

    e = calloc(1, sizeof(Error));
    e->error = err;
    e->msg = strdup(err_msg[err]);
    e->next = resp->errors;
    resp->errors = e;
}
