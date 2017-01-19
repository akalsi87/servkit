/**
 * \file tConnection.c
 * \date Jan 19, 2017
 */

#include <servkit/connection.h>
#include "unittest.h"

#include <servkit/asserts.h>

#include <string.h>
#include <pthread.h>
#include <poll.h>

const int port = 37001;

static skConn server;
static skConn request;

static char const msg[] = "Hello Client!";
#define msgSize sizeof(msg)-1
static char myMsg[msgSize];

void* clientThread(void* arg)
{
    skConn myConn;
    char myErr[SK_CONN_ERR_LEN];
    if (skConnInitTcpClient(myErr, &myConn, "localhost", port) == SK_CONN_ERR) {
        skDbgTraceF(SK_LVL_INFO, "Failed to connect to localhost:%d.", port);
        return "";
    }
    if (skConnRead(myErr, &myConn, myMsg, msgSize) != msgSize) {
        skDbgTraceF(SK_LVL_ERR, "Could not read message on clientThread.");
        return "";
    }
    skDbgTraceF(SK_LVL_INFO, "Received message: %.*s", (int)msgSize, myMsg);
    skConnClose(myErr, &myConn);
    return myMsg;
}

TEST_FUNC( serverAndClientHandshake )
{
    char errBuf[SK_CONN_ERR_LEN];
    pthread_t thrd;
    char const msg[] = "Hello Client!";
    void* res;
    TEST_TRUE(skConnInitTcpServer(errBuf, &server, port, "localhost", 1, 0) == SK_CONN_OK);
    TEST_TRUE(pthread_create(&thrd, 0, clientThread, 0) == 0);
    TEST_TRUE(skConnAccept(errBuf, &server, &request) == SK_CONN_OK);
    TEST_TRUE(skConnWrite(errBuf, &request, msg, msgSize) == msgSize);
    TEST_TRUE(pthread_join(thrd, &res) == 0);
    TEST_TRUE(strcmp((char const*)res, msg) == 0);
    TEST_TRUE(skConnClose(errBuf, &request) == SK_CONN_OK);
    TEST_TRUE(skConnClose(errBuf, &server) == SK_CONN_OK);
}

void SetupTests(void)
{
    REG_TEST( serverAndClientHandshake );
}
