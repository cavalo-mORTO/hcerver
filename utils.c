#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "libctemplate/ctemplate.h"
#include "server.h"

int max ( int a, int b ) { return a > b ? a : b; }
int min ( int a, int b ) { return a < b ? a : b; }

void checkFile(Response *resp, unsigned int permission)
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

    if (permission == 0)
        return;

    if (!(owner & S_IWUSR) || !(group & S_IWGRP))
    {
        addError(resp, FORBIDDEN);
        resp->status = HTTP_FORBIDDEN;
    }

    if (permission == 1)
        return;

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

void getMimeType(Response *resp, char *mime)
{
    char *ext = strrchr(resp->TMPL_file, '.');
    if (!ext)
    {
        strcpy(mime, "text/plain");
        return;
    }

    ext++;
    if (strcmp(ext, "html") == 0)
    {
        char *start = strstr(resp->content, "\"Content-Type\"");
        if (start)
        {
            start += strlen("\"Content-Type\" content=\"");
            char *end = strstr(start, "\"");
            int len = end - start;
            memcpy(mime, start, len);
        }
        else
            strcpy(mime, "text/html");
    }
    else if (strcmp(ext, "css") == 0)
        strcpy(mime, "text/css");
    else if (strcmp(ext, "js") == 0)
        strcpy(mime, "text/javascript");
    else if (strcmp(ext, "csv") == 0)
        strcpy(mime, "text/csv");
    else if ( (strcmp(ext, "jpeg")) == 0 || (strcmp(ext, "jpg")) == 0 )
        strcpy(mime, "image/jpeg");
    else if (strcmp(ext, "png") == 0)
        strcpy(mime, "image/png");
    else if (strcmp(ext, "gif") == 0)
        strcpy(mime, "image/gif");
    else if (strcmp(ext, "bmp") == 0)
        strcpy(mime, "image/bmp");
    else if (strcmp(ext, "svg") == 0)
        strcpy(mime, "image/svg+xml");
    else if (strcmp(ext, "mp4") == 0)
        strcpy(mime, "video/mp4");
    else
        strcpy(mime, "text/plain");
}


void getHttpMetas(char *buffer, char *metas)
{
    if (!strstr(buffer, "<head>"))
        return;

    char *start_tag = strchr(buffer, '<');
    while (start_tag != NULL)
    {
        start_tag++;
        char *end_tag = strchr(start_tag, '>');
        int len = end_tag - start_tag;
        char buf[1024] = {0x0};
        memcpy(buf, start_tag, len);

        /* get attributes in the tag */
        char tag_name[100] = {0x0};
        size_t tag_len = strcspn(buf, " ");
        if (tag_len <= 0)
            tag_len = strlen(buf);

        memcpy(tag_name, buf, tag_len);

        start_tag = strchr(end_tag, '<');

        if (strcmp(tag_name, "meta") != 0)
            continue;

        char key[512] = {0x0};
        char val[512] = {0x0};

        int i = tag_len + 1;
        while (i < strlen(buf))
        {
            char attr_key[128] = {0x0};
            size_t attr_key_len = strcspn(&buf[i], "=");
            memcpy(attr_key, &buf[i], attr_key_len);

            i += attr_key_len + 2;

            char attr_val[512] = {0x0};
            size_t attr_val_len = strcspn(&buf[i], "\"");
            memcpy(attr_val, &buf[i], attr_val_len);

            if (strcmp(attr_key, "content") == 0)
                memcpy(val, attr_val, attr_val_len);
            else if (strcmp(attr_key, "http-equiv") == 0)
                memcpy(key, attr_val, attr_val_len);

            i += attr_val_len + 1;

            while (buf[i] == ' ')
                i++;

            if (buf[i] == '/')
                break;
        }

        if (strlen(key) > 0)
        {
            if (strcmp(key, "Content-Type") == 0)
                continue;

            char buf[1024] = {0x0};
            sprintf(buf, "%s: %s\n", key, val);

            strcat(metas, buf);
        }
    }
}


char *setPath(char *fname)
{
    char *ext = strrchr(fname, '.');
    if (!ext)
        return strdup(fname);

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


char *getRequestArg(Request *req, char *argToFind)
{
    Dict_t *arg = req->queries;
    size_t len = strlen(argToFind);

    if (!arg) return NULL;

    for (; ; arg = arg->next)
    {
        if (memcmp(arg->key, argToFind, len) == 0)
            return arg->value;
    }

    return NULL;
}
