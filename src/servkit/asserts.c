/**
 * \file asserts.c
 * \date Jan 18, 2017
 */

#include <servkit/likely.h>
#include <servkit/asserts.h>

#include <stdio.h>
#include <stdlib.h>

static
char const* removeRootName(char const* file)
{
    int numSeps = 3;
    // skip the ../[src|include|..]/[servkit|servkit|test]
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
    fprintf(stderr, ANSI_COLOR_RED "FAILED ASSERT: " ANSI_COLOR_RESET "(%s:%d) %s\n",
            removeRootName(file), line, msg);
    abort();
    return 0;
}

static FILE* traceFile = 0;

int _skTracePrintFileLine(char const* lvl, char const* file, int line)
{
    fprintf(skGetTraceFile(), "%s(%s:%d)", lvl, removeRootName(file), line);
    return 0;
}

FILE* skGetTraceFile(void)
{
    if (SK_UNLIKELY(traceFile == 0)) {
        traceFile = stderr;
    }
    return traceFile;
}

void skSetTraceFile(FILE* file)
{
    traceFile = file;
}
