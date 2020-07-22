#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "libctemplate/ctemplate.h"
#include "lib.h"


Request *parse_request(char *raw) {
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
        free_request(req);
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
        free_request(req);
        return NULL;
    }
    memcpy(req->route, url_p, route_len);
    url_p += route_len + 1;

    // retrieve query
    Dict *query = NULL, *last_q = NULL;
    while (*url_p)
    {
        size_t arg_len = min(strcspn(url_p, "="), strcspn(url_p, " "));

        last_q = query;
        query = calloc(1, sizeof(Dict));
        if (!query) {
            free_request(req);
            return NULL;
        }

        query->key = calloc(1, arg_len + 1);
        if (!query->key) {
            free_request(req);
            return NULL;
        }
        memcpy(query->key, url_p, arg_len);
        url_p += arg_len + 1;

        size_t val_len = min(strcspn(url_p, "&"), strcspn(url_p, " "));
        query->value = calloc(1, val_len + 1);
        if (!query->value) {
            free_request(req);
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
        free_request(req);
        return NULL;
    }
    memcpy(req->version, raw, ver_len);
    raw += ver_len + 2; // move past <CR><LF>

    Dict *header = NULL, *last = NULL;
    while (raw[0]!='\r' || raw[1]!='\n') {
        last = header;
        header = calloc(1, sizeof(Dict));
        if (!header) {
            free_request(req);
            return NULL;
        }

        // name
        size_t key_len = strcspn(raw, ":");
        header->key = calloc(1, key_len + 1);
        if (!header->key) {
            free_request(req);
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
            free_request(req);
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
        free_request(req);
        return NULL;
    }
    memcpy(req->body, raw, body_len);

    return req;
}
