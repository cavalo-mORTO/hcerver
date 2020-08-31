#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>

#include "libctemplate/ctemplate.h"
#include "server/server.h"



int dinosaurIndexPage(Response *resp, Request *req)
{
    char sql[1024];
    int rc;

    resp->TMPL_file = setPath("dinosaur/index.html");

    char *name = getRequestGetField(req, "name");

    sqlite3_snprintf(sizeof(sql), sql,
            "SELECT * FROM dinosaur WHERE name LIKE '%s%%' ORDER BY name DESC LIMIT 21", name);

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
        char desc_t[256] = {0x0};
        strncpy(desc_t, sqlite3_column_text(stmt, 2), 250);
        strcat(desc_t, "...");

        vl = TMPL_add_var(0,
                "id", sqlite3_column_text(stmt, 0),
                "name", sqlite3_column_text(stmt, 1),
                "desc", desc_t,
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
    if ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
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
