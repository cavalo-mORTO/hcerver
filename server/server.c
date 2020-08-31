#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <grp.h>
#include <regex.h>
#include <sqlite3.h>

#include "../libctemplate/ctemplate.h"
#include "server.h"


static int max ( int a, int b ) { return a > b ? a : b; }
static int min ( int a, int b ) { return a < b ? a : b; }

Response *initResponse()
{
    Response *resp = calloc(1, sizeof(Response));
    if (!resp) return NULL;

    resp->status = SERVER_ERROR_CODE[HTTP_OK].status;

    resp->errors = calloc(128, sizeof(unsigned));
    if (!resp->errors) 
    {
        freeResponse(resp);
        return NULL;
    }

    return resp;
}

static char *getDictValue(Dict_t *d, char *key)
{
    if (key == NULL) return NULL;

    char *value = NULL;
    for (; d != NULL; d = d->next)
    {
        if (strcmp(d->key, key) == 0)
        {
            value = d->value;
            break;
        }
    }

    return value;
}

static char *getMultiFormData(MultiForm_t *m, char *field)
{
    char *data = NULL;
    for (; m != NULL; m = m->next)
    {
        char *name = getDictValue(m->head, "name");
        if (name != NULL && strcmp(name, field) == 0)
        {
            data = m->data;
            break;
        }
    }
    return data;
}

char *getRequestHeader(Request *req, char *header)
{
    return getDictValue(req->headers, header);
}

char *getRequestGetField(Request *req, char *field)
{
    if (req->method != GET) return NULL;

    return getDictValue(req->queries, field);
}

char *getRequestPostField(Request *req, char *field)
{
    if (req->method != POST) return NULL;

    char *type = getRequestHeader(req, "Content-Type");

    if (type != NULL && strncmp(type, "multipart/form-data", 19) == 0)
        return getMultiFormData(req->multi, field);

    return getDictValue(req->form, field);
}


static char *getMimeType(char *fname)
{
    char *ext = strrchr(fname, '.');

    if (!ext)                       return "text/plain";

    ext++;
    if (strcmp(ext, "html") == 0)   return "text/html";
    if (strcmp(ext, "css" ) == 0)   return "text/css";
    if (strcmp(ext, "js"  ) == 0)   return "text/javascript";
    if (strcmp(ext, "csv" ) == 0)   return "text/csv";
    if ( (strcmp(ext, "jpeg")) == 0 || (strcmp(ext, "jpg")) == 0 ) return "image/jpeg";
    if (strcmp(ext, "png" ) == 0)   return "image/png";
    if (strcmp(ext, "gif" ) == 0)   return "image/gif";
    if (strcmp(ext, "bmp" ) == 0)   return "image/bmp";
    if (strcmp(ext, "svg" ) == 0)   return "image/svg+xml";
    if (strcmp(ext, "mp4" ) == 0)   return "video/mp4";

    return "text/plain";
}


static char *makeHeader(Response *resp)
{
    char *mime = getMimeType(resp->TMPL_file);
    time_t now = time(NULL);

    size_t head_len = snprintf(NULL, 0,
            "HTTP/%s %d\n"
            "Content-Type: %s\n"
            "Content-Length: %lu\n"
            "Server: %s\n"
            "Date: %s\n",
            HTTP_VER, resp->status,
            mime,
            resp->content_lenght,
            SERVER_NAME,
            ctime(&now)) + 1;
    
    char *header = calloc(head_len, sizeof(char));
    if (!header) return NULL;

    snprintf(header,
            head_len,
            "HTTP/%s %d\n"
            "Content-Type: %s\n"
            "Content-Length: %lu\n"
            "Server: %s\n"
            "Date: %s\n",
            HTTP_VER, resp->status,
            mime,
            resp->content_lenght,
            SERVER_NAME,
            ctime(&now));

    return header;
}


void renderContent(Response *resp)
{
    addError(resp, readFileOK(resp));

    if (resp->status != SERVER_ERROR_CODE[HTTP_OK].status)
    {
        free(resp->TMPL_file);
        TMPL_free_varlist(resp->TMPL_mainlist);

        TMPL_loop *loop = 0;
        for (unsigned i = 0; resp->errors[i] != 0; i++)
            loop = TMPL_add_varlist(loop, TMPL_add_var(0, "msg", SERVER_ERROR_CODE[resp->errors[i]].msg, 0));

        resp->TMPL_file = setPath("error.html"); 
        resp->TMPL_mainlist = TMPL_add_loop(0, "errors", loop);

        char status[12];
        snprintf(status, sizeof(status), "%d", resp->status);
        resp->TMPL_mainlist = TMPL_add_var(resp->TMPL_mainlist, "status", status, 0);
    }

    resp->content = calloc(4096, sizeof(char));
    TMPL_write(resp->TMPL_file, 0, 0, resp->TMPL_mainlist, &resp->content, 4096, stderr);
    resp->content_lenght = strlen(resp->content);

    resp->header = makeHeader(resp);
}


void addError(Response *resp, unsigned char err)
{
    if (err == HTTP_OK) return;

    unsigned i = 0;
    for (; i < sizeof(resp->errors); i++)
    {
        /* if error already exists quit */
        if (resp->errors[i] == err) return;

        /* if no error is found add to it */
        if (resp->errors[i] == 0) break;
    }

    resp->errors[i] = err;
    resp->status = SERVER_ERROR_CODE[err].status;
}


static void freeDict(Dict_t *d)
{
    if (d == NULL) return;

    freeDict(d->next);
    free(d->key);
    free(d->value);
    free(d);
}

static void freeMultiForm(MultiForm_t *m)
{
    if (m == NULL) return;

    freeMultiForm(m->next);
    freeDict(m->head);
    free(m->data);
    free(m);
}

void freeRequest(Request *req)
{
    freeDict(req->queries);
    freeDict(req->headers);
    freeDict(req->form);
    freeMultiForm(req->multi);
    free(req->route);
    free(req->version);
    free(req->body);
    free(req);
}

void freeResponse(Response *resp)
{
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
    if (!req) return NULL;

    // Method
    size_t meth_len = strcspn(raw, " ");
    if (memcmp(raw, "GET", meth_len) == 0)
        req->method = GET;
    else if (memcmp(raw, "POST", meth_len) == 0)
        req->method = POST;
    else if (memcmp(raw, "HEAD", meth_len) == 0)
        req->method = HEAD;
    else
        req->method = UNSUPPORTED;
    raw += meth_len + 1; // move past <SP>

    // Request-URI
    size_t url_len = strcspn(raw, " ");

    // get the route of url
    size_t route_len = min(strcspn(raw, "?"), url_len);
    req->route = strndup(raw, route_len);

    char *url, *tofree_url;
    url = tofree_url = strndup(raw + route_len + 1, url_len - route_len);
    
    // retrieve query
    Dict_t *query = NULL, *last_q = NULL;
    while (1)
    {
        if (!strchr(url, '=')) break;

        last_q = query;
        query = calloc(1, sizeof(Dict_t));
        if (!query) {
            freeRequest(req);
            return NULL;
        }

        size_t key_len = strcspn(url, "=");
        query->key = strndup(url, key_len);

        url += key_len + 1;
        
        size_t val_len = min(strcspn(url, "&"), strcspn(url, " "));
        query->value = strndup(url, val_len);

        query->next = last_q;

        if (!strchr(url, '&')) break;

        url += val_len + 1;
    }
    free(tofree_url);
    req->queries = query;
    raw += url_len + 1; // move past <SP>

    // HTTP-Version
    size_t ver_len = strcspn(raw, "\r\n");
    req->version = strndup(raw, ver_len);

    raw += ver_len + 2; // move past <CR><LF>

    Dict_t *header = NULL, *last = NULL;
    while (raw[0]!='\r' || raw[1]!='\n')
    {
        last = header;
        header = calloc(1, sizeof(Dict_t));
        if (!header) {
            freeRequest(req);
            return NULL;
        }

        // name
        size_t key_len = strcspn(raw, ":");
        header->key = strndup(raw, key_len);

        raw += key_len + 1; // move past :
        while (*raw == ' ') raw++;

        // value
        size_t value_len = strcspn(raw, "\r\n");
        header->value = strndup(raw, value_len);

        raw += value_len + 2; // move past <CR><LF>

        // next
        header->next = last;
    }
    req->headers = header;
    raw += 2; // move past <CR><LF>

    req->body = strdup(raw);

    if (req->method != POST)
        return req;

    char *content = getRequestHeader(req, "Content-Type");
    if (content != NULL && strncmp(content, "multipart/form-data", 19) == 0)
    {
        char *boundary = strchr(content, '=') - 1;
        boundary[0] = '-';
        boundary[1] = '-';

        raw = strstr(raw, boundary);

        MultiForm_t *multi = NULL, *last_multi = NULL;
        while (raw != NULL && strncmp(raw + strlen(boundary), "--", 2) != 0)
        {
            raw += strlen(boundary);

            last_multi = multi;
            multi = calloc(1, sizeof(MultiForm_t));
            if (!multi)
            {
                freeRequest(req);
                return NULL;
            }

            Dict_t *head = NULL, *last_head = NULL;
            while (1)
            {
                last_head = head;
                head = calloc(1, sizeof(Dict_t));
                if (!head)
                {
                    freeRequest(req);
                    return NULL;
                }

                char *key = strchr(raw, ';') + 1;
                while (key[0] == ' ') key++;

                char *key_end = strchr(key, '=');
                head->key = strndup(key, key_end - key);

                char *value = key_end + 2; 
                char *value_end = strchr(value, '"');

                head->value = strndup(value, value_end - value);
                head->next = last_head;

                raw = value_end + 1;

                if (raw[0] == '\r' || raw[0] == '\n') break;
            }

            while (raw[0] == '\r' || raw[0] == '\n') raw++; 

            char *data = raw;
            char *data_end = strstr(data, boundary);

            if (data_end)
                data_end -= 2; /* discard <CR><LF> */
            else
                data_end = &raw[strlen(raw) - 1];

            multi->head = head;
            multi->data = strndup(data, data_end - data);
            multi->next = last_multi;

            raw = strstr(raw, boundary); /* jump to next field */
        }
        req->multi = multi;
    }
    else
    {
        Dict_t *post = NULL, *last_post = NULL;
        while (1)
        {
            last_post = post;
            post = calloc(1, sizeof(Dict_t));
            if (!post)
            {
                freeRequest(req);
                return NULL;
            };

            char *key = raw; 
            char *key_end = strchr(key, '=');

            char *value = key_end + 1;
            char *value_end = strchr(value, '&') ? strchr(value, '&') : strchr(value, '\0');

            post->key = strndup(key, key_end - key);
            post->value = strndup(value, value_end - value);
            post->next = last_post;

            raw = value_end;

            if (raw[0] == '\0') break;

            raw++;
        }
        req->form = post;
    }

    return req;
}


static int checkFile(const char *fname, char permission)
{
    struct stat st;
    struct group *g;

    /* file does not exist */
    if (lstat(fname, &st) < 0) return HTTP_NOTFOUND;

    /* group does not exist */
    if ((g = getgrnam(PUBLIC_GROUP)) == NULL) return HTTP_FORBIDDEN;

    /* file does not belong to public group */
    if (g->gr_gid != st.st_gid) return HTTP_FORBIDDEN;

    switch(permission)
    {
        case 'r': if ((st.st_mode & S_IRUSR) && (st.st_mode & S_IRGRP)) return HTTP_OK;  
        case 'w': if ((st.st_mode & S_IWUSR) && (st.st_mode & S_IWGRP)) return HTTP_OK;
        case 'x': if ((st.st_mode & S_IXUSR) && (st.st_mode & S_IXGRP)) return HTTP_OK;
    }

    return HTTP_FORBIDDEN;
}

int readFileOK(Response *resp)
{
    const char *fname = resp->TMPL_file ? resp->TMPL_file : "";

    return checkFile(fname, 'r');
}

int writeFileOK(Response *resp)
{
    const char *fname = resp->TMPL_file ? resp->TMPL_file : "";

    return checkFile(fname, 'w');
}

int execFileOK(Response *resp)
{
    const char *fname = resp->TMPL_file ? resp->TMPL_file : "";

    return checkFile(fname, 'x');
}



char *setPath(char *fname)
{
    char *ext = strrchr(fname, '.');
    if (!ext) return strdup(fname);

    ext++;
    char *dir = "";
    if (strcmp(ext, "html") == 0)
        dir = TEMPLATE_DIR;
    else if (strcmp(ext, "js") == 0)
        dir = JS_DIR;
    else if (strcmp(ext, "css") == 0)
        dir = CSS_DIR;

    char *fmt = fname[0] == '/' ? "%s%s" : "%s/%s";
    size_t len = snprintf(NULL, 0, fmt, dir, fname) + 1;

    char *path = calloc(len, sizeof(char));
    snprintf(path, len, fmt, dir, fname);
    
    return path;
}


char *getRouteParam(Request *req, unsigned short pos)
{
    char *ptr = req->route;
    unsigned short i = 0; /* position is zero indexed */

    while (i <= pos)
    {
        if ((ptr = strchr(ptr, '/')) == 0)
            return NULL;

        i++; /* move position */
        ptr++; /* skip "/" */
    }
    char *param = ptr;
    char *param_end = strchr(param, '/') ? strchr(param, '/') : strchr(param, '\0');

    return strndup(param, param_end - param);
}


static int regexMatch(char *matchStr, char *regexStr)
{
    regex_t regex;
    int rc;

    rc = regcomp(&regex, regexStr, 0);
    if (rc) return rc;

    rc = regexec(&regex, matchStr, 0, NULL, 0);
    regfree(&regex);

    return !rc;
}


int routeIs(Request *req, char *route)
{
    return (strcmp(req->route, route) == 0);
}

int routeIsRegEx(Request *req, char *regex)
{
    return regexMatch(req->route, regex);
}
