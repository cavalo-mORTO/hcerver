#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib.h"


struct Response *handle_request(char *raw_request)
{
    Response *resp = calloc(1, sizeof(struct Response));
    resp->args = NULL;

    Request *req = parse_request(raw_request);

    map_route(req, resp);

    compose_response(resp);
    free_request(req);
    return resp;
}


void compose_response(Response *resp)
{
    char *HEADER = "HTTP/1.1 %i\nContent-Type: text/html\nContent-Length: %i\nDate: %s\n\n";
    time_t now = time(NULL);
    char *now_str = ctime(&now);


    int valid = check_file(resp->file);
    switch(valid)
    {
        case 0:
            resp->status = HTTP_OK;
            break;
        case -1:
            resp->status = HTTP_NOTFOUND;
            append_arg(resp, "error", "File not found!");

            free(resp->file);
            resp->file = strdup("static/error.html");

            break;
        case -2:
            resp->status = HTTP_FORBIDDEN;
            append_arg(resp, "error", "No permission to access file!");

            free(resp->file);
            resp->file = strdup("static/error.html");

            break;
    }

    render_content(resp);

    size_t header_size = snprintf(NULL, 0, HEADER, resp->status, resp->content_lenght, now_str) + 1;
    resp->header = malloc(header_size);
    snprintf(resp->header, header_size, HEADER, resp->status, resp->content_lenght, now_str);

    size_t len = resp->content_lenght + header_size;
    resp->repr = malloc(len);
    snprintf(resp->repr, len, "%s%s", resp->header, resp->content);
}


void render_content(struct Response *resp)
{
    char *buf = read_file(resp->file);
    char *prev;
    struct Dict *arg;

    arg = resp->args;
    while(arg != NULL)
    {
        prev = buf;
        buf = str_replace(prev, arg->key, arg->value);
        free(prev);

        arg = arg->next;
    }
    resp->content = buf; 
    resp->content_lenght = strlen(resp->content);
}

void append_arg(struct Response *resp, char *key, char *value)
{
    Dict *arg = calloc(1, sizeof(Dict));
    char *k = calloc(strlen(key) + 5, sizeof(char));
    strcat(k, "//");
    strcat(k, key);
    strcat(k, "//");
    arg->key = k;
    arg->value = strdup(value);
    arg->next = resp->args;

    resp->args = arg;
}
