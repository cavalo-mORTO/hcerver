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

    map_route(req, resp);
    compose_response(resp);

    char *response = calloc(1, strlen(resp->repr) + 1);
    memcpy(response, resp->repr, strlen(resp->repr));

    free_request(req);
    free_response(resp);
    return response;
}


void compose_response(Response *resp)
{
    check_file(resp);

    render_content(resp);
    make_header(resp);

    size_t len = resp->content_lenght + strlen(resp->header) + 1;
    resp->repr = calloc(len, sizeof(char));
    snprintf(resp->repr, len, "%s\n%s", resp->header, resp->content);
}

void make_header(Response *resp)
{
    char status[100] = {0x0};
    sprintf(status, "%d", resp->status);

    char content_len[100] = {0x0};
    sprintf(content_len, "%d", resp->content_lenght);

    time_t now = time(NULL);
    char *now_str = ctime(&now);

    TMPL_varlist *header_list;
    header_list = TMPL_add_var(0,
            "status", status,
            "content_type", "text/html",
            "content_len", content_len,
            "date", now_str, 0);

    resp->header = TMPL_write("templates/headers/header.tmpl", 0, 0, header_list, NULL, stderr);
    TMPL_free_varlist(header_list);
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
    }

    resp->content = TMPL_write(resp->TMPL_file, 0, 0, resp->TMPL_mainlist, NULL, stderr);
    resp->content_lenght = strlen(resp->content);
}


char *error()
{
    char *HEADER = "HTTP/1.1 %i\nContent-Type: text/html\nContent-Length: %i\nDate: %s\n\n";
    time_t now = time(NULL);
    char *now_str = ctime(&now);

    unsigned short int status = 500;

    char *content = "<p>500 Internal Server Error</p>";
    size_t content_lenght = strlen(content); 


    size_t header_size = snprintf(NULL, 0, HEADER, status, content_lenght, now_str) + 1;
    char *header = malloc(header_size);
    snprintf(header, header_size, HEADER, status, content_lenght, now_str);

    size_t len = content_lenght + header_size;
    char *resp = malloc(len + 1);
    snprintf(resp, len, "%s%s", header, content);

    return resp;
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
