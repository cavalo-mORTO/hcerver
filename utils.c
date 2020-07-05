#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lib.h"

int max ( int a, int b ) { return a > b ? a : b; }
int min ( int a, int b ) { return a < b ? a : b; }

char *read_file(const char *fname)
{
    char *buffer = NULL;
    FILE *fp = fopen(fname, "r");
    if (fp != NULL) {
        /* Go to the end of the file. */
        if (fseek(fp, 0L, SEEK_END) == 0) {
            /* Get the size of the file. */
            long bufsize = ftell(fp);
            if (bufsize == -1) {
                return NULL;
            }

            /* Allocate our buffer to that size. */
            buffer = calloc(bufsize + 1, sizeof(char));

            /* Go back to the start of the file. */
            fseek(fp, 0L, SEEK_SET);

            /* Read the entire file into memory. */
            fread(buffer, sizeof(char), bufsize, fp);
        }
        fclose(fp);
    }
    return buffer;
}

int check_file(const char *fname)
{
    if (!fname)
    {
        fputs("File not found!\n", stderr);
        return -1;
    }
    else if (access(fname, F_OK) == -1)
    {
        fputs("File not found!\n", stderr);
        return -1;
    }

    else if (access(fname, R_OK) == -1)
    {
        fputs("No read permision!\n", stderr);
        return -2;
    }

    return 0;
}

char *str_replace(char *input, char *w_old, char *w_new)
{

    size_t i, cnt = 0; 
    size_t w_new_len = strlen(w_new);
    size_t w_old_len = strlen(w_old);
    char *output;

    if (w_old_len < 1)
    {
        output = calloc(strlen(input) + 1, sizeof(char));
        memcpy(output, input, strlen(input));
        return output;
    }

    // Counting the number of times old word occur in the string 
    for (i = 0; input[i] != '\0'; i++) 
    { 
        if (strstr(&input[i], w_old) == &input[i]) 
        { 
            cnt++; 
            i += w_old_len - 1; 
        } 
    }

    if (cnt == 0)
    {
        output = calloc(strlen(input) + 1, sizeof(char));
        memcpy(output, input, strlen(input));
        return output;
    }

    // Making new string of enough length
    output = calloc(i + cnt * (w_new_len - w_old_len) + 1, sizeof(char));

    i = 0;
    char *p = strstr(&input[i], w_old);
    while(p != NULL)
    {
        memcpy(output + strlen(output), &input[i], strlen(&input[i]) - strlen(p));
        memcpy(output + strlen(output), w_new, w_new_len);

        i = w_old_len + strlen(input) - strlen(p);
        p = strstr(&input[i], w_old);

        if (p == NULL)
        {
            memcpy(output + strlen(output), &input[i], strlen(&input[i]));
        }
    }

    return output;
}
