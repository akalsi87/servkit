/**
 * \file bgtaskmanager.h
 * \date Feb 01, 2017
 */

#ifndef _SERVKIT_BGTASKMANAGER_H_
#define _SERVKIT_BGTASKMANAGER_H_

#include <servkit/config.h>

typedef void* skBgTaskPtr;
typedef void* skBgDataPtr;
typedef void (*skBgFunc)(skBgDataPtr, skBgTaskPtr);
typedef skBgDataPtr (*skBgDataCreate)(void*);
typedef void (*skBgDataDestroy)(void*, skBgDataPtr);

typedef struct _skBgTaskNode
{
    struct _skBgTaskNode* prev;
    struct _skBgTaskNode* next;
    skBgTaskPtr task;
} skBgTaskNode;

typedef struct
{
    skBgDataPtr* consumerData;
    void* threads;
    skBgFunc consumerFunc;
    skBgDataDestroy consumerDataDestroyer;
    int numThreads;
    int shutdown;
    void* mutex;
    void* cndVar;
    skBgTaskNode head;
    void* ctxt;
    int consumeRemaining;
} skBgTaskManager;

SK_API
/*!
 *
 */
int skBgTaskManagerInit(skBgTaskManager* mgr, int numThreads, void* ctxt,
                        skBgDataCreate dataCreate,
                        skBgDataDestroy dataDestroy,
                        skBgFunc consumer);

SK_API
/*!
 *
 */
void skBgTaskManagerAddTask(skBgTaskManager* mgr, skBgTaskPtr task);

SK_API
/*!
 *
 */
int skBgTaskManagerDestroy(skBgTaskManager* mgr, int finishTasks);

#endif/*_SERVKIT_BGTASKMANAGER_H_*/
