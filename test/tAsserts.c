/**
 * \file tAsserts.c
 * \date Jan 18, 2017
 */

#include "unittest.h"

#include <signal.h>
#include <setjmp.h>

static int abortCalled = 0;
static int signalNum = -1;
static jmp_buf jmpPoint;

void signalHandler(int sig)
{
    signalNum = sig;
    abortCalled = 1;
    signal(SIGABRT, signalHandler);
    longjmp(jmpPoint, 1);
}

#include <servkit/asserts.h>

TEST_FUNC( asserts )
{
    typedef void(*signalHandleFunc)(int);
    signalHandleFunc oldSig = signal(SIGABRT, signalHandler);
    TEST_TRUE(oldSig != SIG_ERR);
    TEST_TRUE(abortCalled == 0);
    TEST_TRUE(signalNum == -1);
    if (!setjmp(jmpPoint)) {
        skAssert(0);
    } else {
        TEST_TRUE(abortCalled == 1);
        TEST_TRUE(signalNum == SIGABRT);
        abortCalled = 0;
        signalNum = -1;
    }
    if (!setjmp(jmpPoint)) {
        skAssertMsg(0, "Msg");
    } else {
        TEST_TRUE(abortCalled == 1);
        TEST_TRUE(signalNum == SIGABRT);
        abortCalled = 0;
        signalNum = -1;
    }
    TEST_TRUE(signal(SIGABRT, oldSig) != SIG_ERR);
}

TEST_FUNC( traces )
{
    skTrace(SK_INFO, "Info");
    skTrace(SK_WARN, "Warn");
    skTrace(SK_ERR, "Err");
    skTrace(SK_SUCC, "Succ");
    skTraceF(SK_INFO, "Info");
    skTraceF(SK_WARN, "Warn");
    skTraceF(SK_ERR, "Err");
    skTraceF(SK_SUCC, "Succ");

    TEST_TRUE(skGetTraceFile() == stderr);
    skSetTraceFile(stdout);
    TEST_TRUE(skGetTraceFile() == stdout);
    skSetTraceFile(stderr);
}

void SetupTests(void)
{
    REG_TEST( asserts );
    REG_TEST( traces );
}
