/**
 * \file asserts.c
 * \date Jan 18, 2017
 */

#include <servkit/asserts.h>

#include <stdio.h>
#include <stdlib.h>

int _skAssert(char const* msg, char const* file, int line)
{
    fprintf(stderr, "(%s:%d) ASSERTION FAILURE: %s\n", file, line, msg);
    abort();
    return 0;
}
