#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>

#include "libctemplate/ctemplate.h"
#include "server/server.h"


#define JSMN_HEADER
#include "jsmn/jsmn.h"


int dinosaurIndexPage(Response *resp, Request *req)
{
    char sql[1024];
    int rc;

    resp->TMPL_file = setPath("dinosaur/index.html");

    sqlite3_snprintf(sizeof(sql), sql,
            "SELECT * FROM dinosaur WHERE name LIKE '%s%%' ORDER BY name DESC LIMIT 21",
            getRequestGetField(req, "name"));

    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(resp->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("error: %s\n", sqlite3_errmsg(resp->db));
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    TMPL_loop *loop = 0;
    TMPL_varlist *vl;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        char desc[256] = {0x0};
        strncpy(desc, sqlite3_column_text(stmt, 2), 250);
        strcat(desc, "...");

        vl = TMPL_add_var(0,
                "id", sqlite3_column_text(stmt, 0),
                "name", sqlite3_column_text(stmt, 1),
                "desc", desc,
                "img", sqlite3_column_text(stmt, 3), 0);

        loop = TMPL_add_varlist(loop, vl);
    }
    resp->TMPL_mainlist = TMPL_add_loop(resp->TMPL_mainlist, "dinosaurs", loop);

    sqlite3_finalize(stmt);
    return HTTP_OK;
}

int dinosaurShowPage (Response *resp, Request *req)
{
    char sql[1024];
    int rc;

    resp->TMPL_file = setPath("dinosaur/show.html");

    char *temp_id = getRouteParam(req, 2);
    int id = atoi(temp_id);
    free(temp_id);

    sqlite3_snprintf(sizeof(sql), sql,
            "SELECT * FROM dinosaur INNER JOIN content on content.dinosaur_id = dinosaur.id WHERE dinosaur.id = '%d'", id);

    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(resp->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("error: %s\n", sqlite3_errmsg(resp->db));
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    TMPL_loop *loop = 0;
    TMPL_varlist *vl;

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
        return HTTP_NOTFOUND;
    else
    {
        resp->TMPL_mainlist = TMPL_add_var(resp->TMPL_mainlist,
                "id", sqlite3_column_text(stmt, 0),
                "name", sqlite3_column_text(stmt, 1),
                "desc", sqlite3_column_text(stmt, 2),
                "img", sqlite3_column_text(stmt, 3), 0);

        vl = TMPL_add_var(0,
                "sectId", sqlite3_column_text(stmt, 5),
                "sectTitle", sqlite3_column_text(stmt, 6),
                "sectText", sqlite3_column_text(stmt, 7), 0);

        loop = TMPL_add_varlist(loop, vl);

        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            vl = TMPL_add_var(0,
                    "sectId", sqlite3_column_text(stmt, 5),
                    "sectTitle", sqlite3_column_text(stmt, 6),
                    "sectText", sqlite3_column_text(stmt, 7), 0);

            loop = TMPL_add_varlist(loop, vl);
        }
    }

    resp->TMPL_mainlist = TMPL_add_loop(resp->TMPL_mainlist, "sections", loop);
    sqlite3_finalize(stmt);

    sqlite3_snprintf(sizeof(sql), sql,
            "SELECT name, id, parent_id FROM dinosaur WHERE id = @id");

    rc = sqlite3_prepare_v2(resp->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        printf("error: %s\n", sqlite3_errmsg(resp->db));
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    loop = 0;
    while (1)
    {
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_step(stmt);

        const char *name = sqlite3_column_text(stmt, 0);
        const char *self_id = sqlite3_column_text(stmt, 1);
        const char *parent_id = sqlite3_column_text(stmt, 2);

        vl = TMPL_add_var(0,
                "parent_name", name,
                "parent_id", self_id, 0);

        loop = TMPL_add_varlist(loop, vl);

        if (parent_id == NULL) break;

        id = atoi(parent_id);
        sqlite3_reset(stmt);
    }
    resp->TMPL_mainlist = TMPL_add_loop(resp->TMPL_mainlist, "parents", loop);

    sqlite3_finalize(stmt);
    return HTTP_OK;
}


int dinosaurCreatePage (Response *resp, Request *req)
{
    char *s;
    int i;
    int r;
    jsmn_parser p;
    jsmntok_t t[128]; /* We expect no more than 128 tokens */

    s = req->body;

    jsmn_init(&p);
    r = jsmn_parse(&p, s, strlen(s), t,
                 sizeof(t) / sizeof(t[0]));

    if (r < 0) {
        printf("Failed to parse JSON: %d\n", r);
        return 1;
    }

    /* Assume the top-level element is an object */
    if (r < 1 || t[0].type != JSMN_OBJECT) {
        printf("Object expected\n");
        return 1;
    }

    char *name;
    char *parent;
    char *img;
    char *desc;

    for (i = 1; i < r; i++)
    {
        if (jsoneq(s, &t[i], "name") == 0)
            name = jsoncpy(s, &t[++i]);
        else if (jsoneq(s, &t[i], "parent") == 0)
            parent = jsoncpy(s, &t[++i]);
        else if (jsoneq(s, &t[i], "img") == 0)
            img = jsoncpy(s, &t[++i]);
        else if (jsoneq(s, &t[i], "text") == 0)
            desc = jsoncpy(s, &t[++i]);
    }
    
    sqlite3_stmt *stmt;
    char sql[30000];
    sqlite3_snprintf(sizeof(sql), sql,
            "SELECT id FROM dinosaur WHERE name LIKE '%s'", parent);

    sqlite3_prepare_v2(resp->db, sql, -1, &stmt, NULL);
    sqlite3_step(stmt);
    int parent_id = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    sqlite3_snprintf(sizeof(sql), sql,
            "INSERT INTO dinosaur VALUES (NULL, '%s', '%s', '%s', '%d')", name, desc, img, parent_id);
    sqlite3_exec(resp->db, sql, NULL, NULL, NULL);

    free(name);
    free(parent);
    free(img);
    free(desc);

    int dino_id = sqlite3_last_insert_rowid(resp->db);
    sqlite3_snprintf(sizeof(sql), sql,
            "INSERT INTO content VALUES (NULL, @title, @text, @id)"); 
    sqlite3_prepare_v2(resp->db, sql, -1, &stmt, NULL);


    sqlite3_exec(resp->db, "BEGIN TRANSACTION", NULL, NULL, NULL);

    /* Loop over all keys of the root object */
    int bound = 0;
    for (i = 1; i < r; i++)
    {
        if (jsoneq(s, &t[i], "title") == 0)
        {
            i++;
            char *str = jsoncpy(s, &t[i]);
            sqlite3_bind_text(stmt, 1, str, -1, SQLITE_TRANSIENT);
            free(str);
            bound++;
        }
        if (jsoneq(s, &t[i], "text") == 0)
        {
            i++;
            char *str = jsoncpy(s, &t[i]);
            sqlite3_bind_text(stmt, 2, str, -1, SQLITE_TRANSIENT);
            free(str);
            bound++;
        }

        if (bound == 2)
        {
            sqlite3_bind_int(stmt, 3, dino_id);
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
            bound = 0;
        }
    }

    sqlite3_exec(resp->db, "END TRANSACTION", NULL, NULL, NULL);
    sqlite3_finalize(stmt);

    return HTTP_OK;
}
