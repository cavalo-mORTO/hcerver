#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include <unistd.h>
#include <sys/stat.h>
#include <regex.h>


#include "../libctemplate/ctemplate.h"
#include "server.h"


int max ( int a, int b ) { return a > b ? a : b; }
int min ( int a, int b ) { return a < b ? a : b; }


static char *getMimeType(char *fname)
{
    char *ext = strrchr(fname, '.');
    if (!ext) return "text/plain";

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


static void makeHeader(Response *resp)
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
    
    resp->header = calloc(head_len + 1, sizeof(char));
    sprintf(resp->header,
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
}


void renderContent(Response *resp)
{
    readFileOK(resp);

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

    makeHeader(resp);
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
        header->key = calloc(1, key_len + 1);
        if (!header->key) {
            freeRequest(req);
            return NULL;
        }
        memcpy(header->key, raw, key_len);
        raw += key_len + 1; // move past :
        while (*raw == ' ') raw++;

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

    if (req->method == POST)
    {
        Dict_t *post = NULL, *lastPost = NULL;

        char *token;
        while ((token = strsep(&raw, "&")) != NULL)
        {
            lastPost = post;
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
            post->next = lastPost;

            free(tofree);
        }
        req->posts = post;
    }

    return req;
}



static void checkFile(Response *resp, unsigned int permission)
{
    const char *fname = resp->TMPL_file ? resp->TMPL_file : "";
    struct stat st;
    mode_t owner, group;
    uid_t usrID = getuid();
    gid_t grpID = getgid();

    if (lstat(fname, &st) < 0)
    {
        addError(resp, NO_FILE);
        resp->status = HTTP_NOTFOUND;
        return;
    }

    owner = st.st_mode & S_IRWXU;
    group = st.st_mode & S_IRWXG;
    
    if (usrID != st.st_uid || grpID != st.st_gid)
    {
        addError(resp, FORBIDDEN);
        resp->status = HTTP_FORBIDDEN;
        return;
    }

    if (!(owner & S_IRUSR) || !(group & S_IRGRP))
    {
        addError(resp, FORBIDDEN);
        resp->status = HTTP_FORBIDDEN;
    }

    if (permission == 0) return;

    if (!(owner & S_IWUSR) || !(group & S_IWGRP))
    {
        addError(resp, FORBIDDEN);
        resp->status = HTTP_FORBIDDEN;
    }

    if (permission == 1) return;

    if (!(owner & S_IXUSR) || !(group & S_IXGRP))
    {
        addError(resp, FORBIDDEN);
        resp->status = HTTP_FORBIDDEN;
    }
}

void readFileOK(Response *resp)
{
    checkFile(resp, 0);
}

void writeFileOK(Response *resp)
{
    checkFile(resp, 1);
}

void execFileOK(Response *resp)
{
    checkFile(resp, 2);
}



char *setPath(char *fname)
{
    char *ext = strrchr(fname, '.');
    if (!ext) return strdup(fname);

    ext++;
    char *dir;
    if (strcmp(ext, "html") == 0)
        dir = TEMPLATE_DIR;
    else if (strcmp(ext, "js") == 0)
        dir = JS_DIR;
    else if (strcmp(ext, "css") == 0)
        dir = CSS_DIR;
    else 
        dir = "";

    char *fmt = fname[0] == '/' ? "%s%s" : "%s/%s";

    size_t len = snprintf(NULL, 0, fmt, dir, fname);

    char *path = calloc(len + 1, sizeof(char));
    sprintf(path, fmt, dir, fname);
    
    return path;
}


static char *getRequestArg(Dict_t *arg, char *argToFind)
{
    if (!arg) return NULL;

    do
    {
        if (strcmp(arg->key, argToFind) == 0) return arg->value;
        arg = arg->next;
    }
    while (arg != NULL);

    return NULL;
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

int regexMatch(char *regexStr, char *matchStr)
{
    regex_t regex;
    int rc;

    rc = regcomp(&regex, regexStr, 0);
    if (rc != 0) return 1;

    rc = regexec(&regex, matchStr, 0, NULL, 0);
    regfree(&regex);

    if (rc == 0) return 0;

    return 1;
}


int routeIs(Request *req, char *route)
{
    return !strcmp(req->route, route);
}

int routeIsRegEx(Request *req, char *regex)
{
    return regexMatch(req->route, regex);
}
