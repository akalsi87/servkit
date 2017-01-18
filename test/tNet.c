/**
 * \file tNet.c
 * \date Jan 18, 2017
 */

#include "unittest.h"

#include <servkit/net.h>
#include <servkit/asserts.h>

TEST_FUNC( no_op )
{
    TEST_TRUE( 1 );
    skTrace(SK_INFO, "Info");
    skTrace(SK_WARN, "Warn");
    skTrace(SK_ERR, "Err");
    skTrace(SK_SUCC, "Succ");
    skTraceF(SK_INFO, "Info");
    skTraceF(SK_WARN, "Warn");
    skTraceF(SK_ERR, "Err");
    skTraceF(SK_SUCC, "Succ");
}

void SetupTests(void)
{
    REG_TEST( no_op );
}
