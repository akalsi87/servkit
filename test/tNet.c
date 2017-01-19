/**
 * \file tNet.c
 * \date Jan 18, 2017
 */

#include "unittest.h"

#include <servkit/net.h>

TEST_FUNC( createTCPServerAndClose )
{
    char errBuf[SK_NET_ERR_LEN];
    char domain[] = "localhost";
    int port = 30700;
    int server;
    //int client;

    server = skNetTcpServer(errBuf, port, domain, 1);
    TEST_TRUE(server != SK_NET_ERR);
    TEST_TRUE(skNetClose(errBuf, server) == SK_NET_OK);
}

void SetupTests(void)
{
    REG_TEST( createTCPServerAndClose );
}
