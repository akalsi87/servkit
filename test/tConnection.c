/**
 * \file tConnection.c
 * \date Jan 19, 2017
 */

#include <servkit/connection.h>
#include "unittest.h"

#include <servkit/asserts.h>

#include <pthread.h>

const int port = 37001;

static skConn server;
static skConn request;

void* clientThread(void* arg)
{
    skConn myConn;
    char myErr[SK_CONN_ERR_LEN];
    while (skConnInitTcpClient(myErr, &myConn, "localhost", port) == SK_CONN_ERR) {
        skDbgTraceF(SK_LVL_INFO, "Failed to connect to localhost:%d.", port);
    }
    skConnClose(myErr, &myConn);
    return 0;
}

TEST_FUNC( serverAndClientHandshake )
{
    char errBuf[SK_CONN_ERR_LEN];
    pthread_t thrd;
    TEST_TRUE(skConnInitTcpServer(errBuf, &server, port, 0, 1, 0) == SK_CONN_OK);
    TEST_TRUE(pthread_create(&thrd, 0, clientThread, 0) == 0);
    TEST_TRUE(skConnAccept(errBuf, &server, &request) == SK_CONN_OK);
    TEST_TRUE(pthread_join(thrd,0) == 0);
    TEST_TRUE(skConnClose(errBuf, &request) == SK_CONN_OK);
    TEST_TRUE(skConnClose(errBuf, &server) == SK_CONN_OK);
}

void SetupTests(void)
{
    REG_TEST( serverAndClientHandshake );
}
