#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lib.h"

int max ( int a, int b ) { return a > b ? a : b; }
int min ( int a, int b ) { return a < b ? a : b; }

char *read_file(const char *fname)
{
    // make sure file exists before calling this function!
    char *buffer = NULL;

    if (access(fname, F_OK) == -1)
    {
        fputs("Could not read file!\n", stderr);
        return buffer;
    }

    FILE *fp = fopen(fname, "r");
    if (fp != NULL) {
        /* Go to the end of the file. */
        if (fseek(fp, 0L, SEEK_END) == 0) {
            /* Get the size of the file. */
            long bufsize = ftell(fp);
            if (bufsize == -1) { /* Error */ }

            /* Allocate our buffer to that size. */
            buffer = malloc(bufsize + 1);

            /* Go back to the start of the file. */
            if (fseek(fp, 0L, SEEK_SET) != 0) { /* Error */ }

            /* Read the entire file into memory. */
            size_t newLen = fread(buffer, sizeof(char), bufsize, fp);
            if ( ferror( fp ) != 0 ) {
                fputs("Error reading file\n", stderr);
            } else {
                buffer[newLen++] = '\0'; /* Just to be safe. */
            }
        }
        fclose(fp);
    }
    return buffer;
}

int check_file(const char *fname)
{
    if (access(fname, F_OK) == -1)
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

