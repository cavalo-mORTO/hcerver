#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib.h"

struct Request *parse_request(const char *raw) {
    struct Request *req = NULL;
    req = malloc(sizeof(struct Request));
    if (!req) {
        return NULL;
    }
    memset(req, 0, sizeof(struct Request));

    // Method
    size_t meth_len = strcspn(raw, " ");
    if (memcmp(raw, "GET", strlen("GET")) == 0) {
        req->method = GET;
    } else if (memcmp(raw, "HEAD", strlen("HEAD")) == 0) {
        req->method = HEAD;
    } else {
        req->method = UNSUPPORTED;
    }
    raw += meth_len + 1; // move past <SP>

    // Request-URI
    size_t url_len = strcspn(raw, " ");

    // give url buffer more memory than needed so that in the loop we don't go out of bounds
    char *url = malloc(url_len + 10);

    // initialize url buffer
    memset(url, 0, url_len + 10);

    // copy unto it the contents of raw
    memcpy(url, raw, url_len);
    url[url_len] = '\0';

    // set pointer for loop
    char *url_p = url;

    // get the route of url
    size_t route_len = min(strcspn(url_p, "?"), strcspn(url_p, " "));
    req->route = malloc(route_len + 1);

    if (!req->route) {
        free_request(req);
        return NULL;
    }
    memcpy(req->route, url_p, route_len);
    req->route[route_len] = '\0';

    url_p += route_len + 1;

    // retrieve query
    struct Header *query = NULL, *last_q = NULL;
    while (*url_p)
    {
        size_t arg_len = min(strcspn(url_p, "="), strcspn(url_p, " "));

        last_q = query;
        query = malloc(sizeof(Header));
        if (!query) {
            free_request(req);
            return NULL;
        }

        query->name = malloc(arg_len + 1);
        if (!query->name) {
            free_request(req);
            return NULL;
        }
        memcpy(query->name, url_p, arg_len);
        query->name[arg_len] = '\0';

        url_p += arg_len + 1;

        size_t val_len = min(strcspn(url_p, "&"), strcspn(url_p, " "));
        query->value = malloc(val_len + 1);
        if (!query->value) {
            free_request(req);
            return NULL;
        }
        memcpy(query->value, url_p, val_len);
        query->value[val_len] = '\0';
        url_p += val_len + 1;

        query->next = last_q;
    }
    req->queries = query;
    free(url);

    raw += url_len + 1; // move past <SP>

    // HTTP-Version
    size_t ver_len = strcspn(raw, "\r\n");
    req->version = malloc(ver_len + 1);
    if (!req->version) {
        free_request(req);
        return NULL;
    }
    memcpy(req->version, raw, ver_len);
    req->version[ver_len] = '\0';
    raw += ver_len + 2; // move past <CR><LF>

    struct Header *header = NULL, *last = NULL;
    while (raw[0]!='\r' || raw[1]!='\n') {
        last = header;
        header = malloc(sizeof(Header));
        if (!header) {
            free_request(req);
            return NULL;
        }

        // name
        size_t name_len = strcspn(raw, ":");
        header->name = malloc(name_len + 1);
        if (!header->name) {
            free_request(req);
            return NULL;
        }
        memcpy(header->name, raw, name_len);
        header->name[name_len] = '\0';
        raw += name_len + 1; // move past :
        while (*raw == ' ') {
            raw++;
        }

        // value
        size_t value_len = strcspn(raw, "\r\n");
        header->value = malloc(value_len + 1);
        if (!header->value) {
            free_request(req);
            return NULL;
        }
        memcpy(header->value, raw, value_len);
        header->value[value_len] = '\0';
        raw += value_len + 2; // move past <CR><LF>

        // next
        header->next = last;
    }
    req->headers = header;
    raw += 2; // move past <CR><LF>

    size_t body_len = strlen(raw);
    req->body = malloc(body_len + 1);
    if (!req->body) {
        free_request(req);
        return NULL;
    }
    memcpy(req->body, raw, body_len);
    req->body[body_len] = '\0';

    return req;
}


void compose_response(Response *resp)
{
    char *HEADER = "HTTP/1.1 %i\nContent-Type: text/html\nContent-Length: %i\n\n";

    int valid = check_file(resp->file);
    if (valid == 0)
    {
        resp->status = OK;
    }
    else if (valid == -1)
    {
        resp->status = NOTFOUND;
    }
    else
    {
        resp->status = FORBIDDEN;
    }

    if (resp->status == OK) {
        resp->content = read_file(resp->file);
    } else {
        resp->content_lenght = snprintf(NULL, 0, TMPL, resp->status, resp->status) + 1;
        resp->content = malloc(resp->content_lenght);
        snprintf(resp->content, resp->content_lenght, TMPL, resp->status, resp->status);
    }

    resp->content_lenght = strlen(resp->content);


    size_t header_size = snprintf(NULL, 0, HEADER, resp->status, resp->content_lenght) + 1;
    resp->header = malloc(header_size);
    snprintf(resp->header, header_size, HEADER, resp->status, resp->content_lenght);

    size_t len = resp->content_lenght + header_size;
    resp->repr = malloc(len);
    snprintf(resp->repr, len, "%s%s", resp->header, resp->content);
}


struct Response *handle_request(const char *raw_request)
{
    Response *resp = NULL;
    resp = malloc(sizeof(struct Response));
    memset(resp, 0, sizeof(struct Response));

    Request *req = parse_request(raw_request);

    // check routes.c
    map_route(req, resp);
    
    compose_response(resp);
    free_request(req);
    return resp;
}


void free_header(struct Header *h) {
    if (h) {
        free(h->name);
        free(h->value);
        free_header(h->next);
        free(h);
    }
}

void free_request(struct Request *req) {
    free(req->route);
    free(req->version);
    free_header(req->queries);
    free_header(req->headers);
    free(req->body);
    free(req);
}

void free_response(struct Response *resp) {
    free(resp->content);
    free(resp->header);
    free(resp->repr);
    free(resp);
}
