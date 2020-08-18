#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <grp.h>
#include <regex.h>


#include "../libctemplate/ctemplate.h"
#include "server.h"


static int max ( int a, int b ) { return a > b ? a : b; }
static int min ( int a, int b ) { return a < b ? a : b; }


static char *getMimeType(char *fname)
{
    char *ext = strrchr(fname, '.') ? strrchr(fname, '.') : "something";

    ext++;
    if (strcmp(ext, "html"    ) == 0) return "text/html";
    else if (strcmp(ext, "css") == 0) return "text/css";
    else if (strcmp(ext, "js" ) == 0) return "text/javascript";
    else if (strcmp(ext, "csv") == 0) return "text/csv";
    else if ( (strcmp(ext, "jpeg")) == 0 || (strcmp(ext, "jpg")) == 0 ) return "image/jpeg";
    else if (strcmp(ext, "png") == 0) return "image/png";
    else if (strcmp(ext, "gif") == 0) return "image/gif";
    else if (strcmp(ext, "bmp") == 0) return "image/bmp";
    else if (strcmp(ext, "svg") == 0) return "image/svg+xml";
    else if (strcmp(ext, "mp4") == 0) return "video/mp4";

    return "text/plain";
}


static char *makeHeader(Response *resp)
{
    char *mime = getMimeType(resp->TMPL_file);
    time_t now = time(NULL);

    size_t head_len = snprintf(NULL, 0,
            "HTTP/%s %d\n"
            "Content-Type: %s\n"
            "Content-Length: %ld\n"
            "Server: %s\n"
            "Date: %s\n",
            HTTP_VER, resp->status,
            mime,
            resp->content_lenght,
            SERVER_NAME,
            ctime(&now));
    
    char *header = calloc(head_len + 1, sizeof(char));
    if (!header) return NULL;

    sprintf(header,
            "HTTP/%s %d\n"
            "Content-Type: %s\n"
            "Content-Length: %ld\n"
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
    switch(readFileOK(resp))
    {
        case -1:
            addError(resp, NO_FILE);
            resp->status = HTTP_NOTFOUND;
            break;
        case 1:
            addError(resp, FORBIDDEN);
            resp->status = HTTP_FORBIDDEN;
            break;
    }

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

    resp->content = calloc(1000, sizeof(char));
    TMPL_write(resp->TMPL_file, 0, 0, resp->TMPL_mainlist, &resp->content, stderr);
    resp->content_lenght = strlen(resp->content);

    resp->header = makeHeader(resp);
}


void addError(Response *resp, unsigned char err)
{
    Error_t *e;
    for (e = resp->errors; e; e = e->next)
    {
        if (e->error == err)
            return;
    }

    e = calloc(1, sizeof(Error_t));
    e->error = err;
    e->msg = SERVER_ERROR_MSG[err];
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


static void freeDict(Dict_t *d)
{
    if (d)
    {
        freeDict(d->next);
        free(d->key);
        free(d->value);
        free(d);
    }
}

static void freeError(Error_t *e)
{
    if (e)
    {
        freeError(e->next);
        free(e);
    }
}

void freeRequest(Request *req)
{
    freeDict(req->queries);
    freeDict(req->headers);
    freeDict(req->posts);
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
    req->route = calloc(route_len + 1, sizeof(char));
    if (!req->route) {
        freeRequest(req);
        return NULL;
    }
    memcpy(req->route, raw, route_len);

    char *url, *tofree_url;
    url = tofree_url = calloc(1 + url_len - route_len, sizeof(char));
    if (!url)
    {
        freeRequest(req);
        return NULL;
    }
    memcpy(url, raw + route_len + 1, url_len - route_len);

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
        query->key = calloc(key_len + 1, sizeof(char));
        if (!query->key) {
            freeRequest(req);
            return NULL;
        }
        memcpy(query->key, url, key_len);
        url += key_len + 1;
        
        size_t val_len = min(strcspn(url, "&"), strcspn(url, " "));
        query->value = calloc(val_len + 1, sizeof(char));
        if (!query->value) {
            freeRequest(req);
            return NULL;
        }
        memcpy(query->value, url, val_len);
        query->next = last_q;

        if (!strchr(url, '&')) break;

        url += val_len + 1;
    }
    free(tofree_url);
    req->queries = query;
    raw += url_len + 1; // move past <SP>

    // HTTP-Version
    size_t ver_len = strcspn(raw, "\r\n");
    req->version = calloc(ver_len + 1, sizeof(char));
    if (!req->version) {
        freeRequest(req);
        return NULL;
    }
    memcpy(req->version, raw, ver_len);
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
        header->key = calloc(key_len + 1, sizeof(char));
        if (!header->key) {
            freeRequest(req);
            return NULL;
        }
        memcpy(header->key, raw, key_len);
        raw += key_len + 1; // move past :
        while (*raw == ' ') raw++;

        // value
        size_t value_len = strcspn(raw, "\r\n");
        header->value = calloc(value_len + 1, sizeof(char));
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
    req->body = calloc(body_len + 1, sizeof(char));
    if (!req->body) {
        freeRequest(req);
        return NULL;
    }
    memcpy(req->body, raw, body_len);

    Dict_t *post = NULL, *last_post = NULL;
    if (req->method == POST)
    {
        char *token;
        while ((token = strsep(&raw, "&")) != NULL)
        {
            last_post = post;
            post = calloc(1, sizeof(Dict_t));
            if (!post)
            {
                freeRequest(req);
                return NULL;
            };

            char *string, *tofree;
            string = tofree = strdup(token);

            char *key = strsep(&string, "=");
            char *val = string;

            post->key = strdup(key);
            post->value = strdup(val);
            post->next = last_post;

            free(tofree);
        }
    }
    req->posts = post;

    return req;
}



static int checkFile(const char *fname, char permission)
{
    struct stat st;
    struct group *g;
    int forbidden = 1;

    /* file does not exist */
    if (lstat(fname, &st) < 0) return -1;

    /* group does not exist */
    if ((g = getgrnam(PUBLIC_GROUP)) == NULL) return forbidden;

    /* file does not belong to public group */
    if (g->gr_gid != st.st_gid) return forbidden;

    switch(permission)
    {
        case 'r': if ((st.st_mode & S_IRUSR) && (st.st_mode & S_IRGRP)) forbidden = 0; break;
        case 'w': if ((st.st_mode & S_IWUSR) && (st.st_mode & S_IWGRP)) forbidden = 0; break;
        case 'x': if ((st.st_mode & S_IXUSR) && (st.st_mode & S_IXGRP)) forbidden = 0; break;
    }

    return forbidden;
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
    size_t len = snprintf(NULL, 0, fmt, dir, fname);

    char *path = calloc(len + 1, sizeof(char));
    sprintf(path, fmt, dir, fname);
    
    return path;
}


static char *getRequestArg(Dict_t *arg, char *argToFind)
{
    char *value = NULL;

    while (arg != NULL)
    {
        if (strcmp(arg->key, argToFind) == 0) value = arg->value;
        arg = arg->next;
    }

    return value;
}

char *getRequestUrlArg(Request *req, char *argToFind)
{
    return getRequestArg(req->queries, argToFind);
}


char *getRequestPostArg(Request *req, char *argToFind)
{
    return getRequestArg(req->posts, argToFind);
}


char *getRouteParam(Request *req, unsigned int pos)
{
    char *token, *string, *tofree;
    int i = 0;

    tofree = string = strdup(req->route);
    while ((token = strsep(&string, "/")) != NULL)
    {
        if (i > pos) break;
        i++;
    }
    char *toReturn = strdup(token);

    free(tofree);
    return toReturn;
}

static int regexMatch(char *matchStr, char *regexStr)
{
    regex_t regex;
    int rc;

    rc = regcomp(&regex, regexStr, 0);
    if (rc != 0) return 1;

    rc = regexec(&regex, matchStr, 0, NULL, 0);
    regfree(&regex);

    return !rc;
}


int routeIs(Request *req, char *route)
{
    return !strcmp(req->route, route);
}

int routeIsRegEx(Request *req, char *regex)
{
    return regexMatch(req->route, regex);
}
