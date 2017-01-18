/**
 * \file asserts.c
 * \date Jan 18, 2017
 */

#include <servkit/asserts.h>

#include <stdio.h>
#include <stdlib.h>

static
char const * localFile(char const* file)
{
    int numSeps = 2;
    // skip the ../<dir>/
    while (*file != '\0' && numSeps > 0) {
        if (*file == '/' || *file == '\\') {
            --numSeps;
        }
        ++file;
    }
    return file;
}

int _skAssert(char const* msg, char const* file, int line)
{
    fprintf(stderr, "(%s:%d) ASSERTION FAILURE: %s\n", localFile(file), line, msg);
    abort();
    return 0;
}

int _skTracePrintFileLine(char const* file, int line)
{
    fprintf(stderr, "(%s:%d)", localFile(file), line);
    return 0;
}
