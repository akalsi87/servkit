/**
 * \file asserts.c
 * \date Jan 18, 2017
 */

#include <servkit/asserts.h>

#include <stdio.h>
#include <stdlib.h>

int _skAssert(char const* msg, char const* file, int line)
{
    int const relativeFileSkip = (sizeof("../src/")/sizeof(char)) - /*for the \0*/1;
    fprintf(stderr, "(%s:%d) ASSERTION FAILURE: %s\n", file+relativeFileSkip, line, msg);
    abort();
    return 0;
}
