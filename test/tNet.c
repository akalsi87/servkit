/**
 * \file tNet.c
 * \date Jan 18, 2017
 */

#include "unittest.h"

#include <servkit/net.h>

TEST_FUNC( no_op )
{
    TEST_TRUE( 1 );
}

void SetupTests(void)
{
    REG_TEST( no_op );
}
