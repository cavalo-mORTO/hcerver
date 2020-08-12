#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>

#include "libctemplate/ctemplate.h"
#include "server.h"

struct dinosaur 
{
    char *id;
    char *name;
    char *desc;
    char *img;
    struct dinosaur *next;
};

void free_dinosaurs(struct dinosaur *d)
{
    if (d)
    {
        free_dinosaurs(d->next);
        free(d->id);
        free(d->name);
        free(d->desc);
        free(d->img);
        free(d);
    }
}

void indexPage(Response *resp)
{
    resp->TMPL_file = setPath("index.html");
}


void dinosaurIndexPage(Response *resp, Request *req)
{
    resp->TMPL_file = setPath("dinosaur/index.html");

    char *a = getRequestArg(req, "name");

    sqlite3 *db;
    int rc = sqlite3_open("test.db", &db);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);

        return;
    }

    const char *sql_t = "SELECT * FROM dinosaur WHERE name LIKE '%s%%' ORDER BY name DESC LIMIT 21";
    char sql[1024];
    sqlite3_snprintf(1024, sql, sql_t, a);

    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("error: %s\n", sqlite3_errmsg(db));
        return;
    }

    struct dinosaur *d = NULL;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        struct dinosaur *tmp = calloc(1, sizeof(struct dinosaur));
        const char *id = sqlite3_column_text(stmt, 0);
        const char *name = sqlite3_column_text(stmt, 1);
        const char *desc = sqlite3_column_text(stmt, 2);
        const char *img = sqlite3_column_text(stmt, 3);

        tmp->id = calloc(strlen(id) + 1, sizeof(char));
        memcpy(tmp->id, id, strlen(id));
        tmp->name = calloc(strlen(name) + 1, sizeof(char));
        memcpy(tmp->name, name, strlen(name));
        tmp->desc = calloc(strlen(desc) + 1, sizeof(char));
        memcpy(tmp->desc, desc, strlen(desc));
        tmp->img = calloc(strlen(img) + 1, sizeof(char));
        memcpy(tmp->img, img, strlen(img));

        tmp->next = d;
        d = tmp;
    }

    if (rc != SQLITE_DONE)
    {
        printf("error: %s\n", sqlite3_errmsg(db));
    }


    TMPL_loop *loop = 0;
    TMPL_varlist *vl;
    for (struct dinosaur *ptr = d; ptr; ptr = ptr->next)
    {
        if (!ptr) break;

        char desc[256] = {0x0};
        memcpy(desc, ptr->desc, min(250, strlen(ptr->desc)));
        strcat(desc, "...");

        vl = TMPL_add_var(0, "id", ptr->id, "name", ptr->name, "desc", desc, "img", ptr->img, 0);
        loop = TMPL_add_varlist(loop, vl);
    }
    resp->TMPL_mainlist = TMPL_add_loop(resp->TMPL_mainlist, "dinosaurs", loop);

    free_dinosaurs(d);

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void dinosaurShowPage (Response *resp, Request *req)
{
    resp->TMPL_file = setPath("dinosaur/show.html");

    sqlite3 *db;
    int rc = sqlite3_open("test.db", &db);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);

        return;
    }

    char *a = getRouteParam(req, 2);
    a = a ? a : "";

    const char *sql_t = "SELECT * FROM dinosaur INNER JOIN content on content.dinosaur_id = dinosaur.id WHERE dinosaur.id = '%s'"; 
    char sql[1024];
    sqlite3_snprintf(1024, sql, sql_t, a);

    free(a);

    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("error: %s\n", sqlite3_errmsg(db));
        return;
    }


    TMPL_loop *loop = 0;
    TMPL_varlist *vl;

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        const char *id = sqlite3_column_text(stmt, 0);
        const char *name = sqlite3_column_text(stmt, 1);
        const char *desc = sqlite3_column_text(stmt, 2);
        const char *img = sqlite3_column_text(stmt, 3);

        const char *sectId = sqlite3_column_text(stmt, 5);
        const char *sectTitle = sqlite3_column_text(stmt, 6);
        const char *sectText = sqlite3_column_text(stmt, 7);

        resp->TMPL_mainlist = TMPL_add_var(resp->TMPL_mainlist, "id", id, "name", name, "desc", desc, "img", img, 0);
        vl = TMPL_add_var(0, "sectId", sectId, "sectTitle", sectTitle, "sectText", sectText, 0);
        loop = TMPL_add_varlist(loop, vl);
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        const char *sectId = sqlite3_column_text(stmt, 5);
        const char *sectTitle = sqlite3_column_text(stmt, 6);
        const char *sectText = sqlite3_column_text(stmt, 7);

        vl = TMPL_add_var(0, "sectId", sectId, "sectTitle", sectTitle, "sectText", sectText, 0);
        loop = TMPL_add_varlist(loop, vl);
    }
    resp->TMPL_mainlist = TMPL_add_loop(resp->TMPL_mainlist, "sections", loop);

    if (rc != SQLITE_DONE)
        printf("error: %s\n", sqlite3_errmsg(db));

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return;
}



void helloPage(Response *resp)
{
    resp->TMPL_file = setPath("index");
    resp->TMPL_mainlist = TMPL_add_var(resp->TMPL_mainlist, "hello", "Hello world!", 0);
}




void mapRoute(Request *req, Response *resp)
{
    if (strcmp(req->route, "/") == 0)
        indexPage(resp);
    else if (strcmp(req->route, "/hello") == 0)
        helloPage(resp);
    else if (strcmp(req->route, "/dinosaur") == 0)
        dinosaurIndexPage(resp, req);
    else if (regexMatch("/dinosaur/show/[0-9]*$", req->route) == 0)
        dinosaurShowPage(resp, req);
    else
    {
        resp->status = HTTP_NOTFOUND;
        addError(resp, NO_ROUTE);
    }
}
