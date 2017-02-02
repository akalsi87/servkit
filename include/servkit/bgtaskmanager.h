/**
 * \file bgtaskmanager.h
 * \date Feb 01, 2017
 */

#ifndef _SERVKIT_BGTASKMANAGER_H_
#define _SERVKIT_BGTASKMANAGER_H_

#include <servkit/config.h>

typedef void* skBgConsumerTaskPtr;
typedef void* skBgConsumerDataPtr;
typedef void (*skBgConsumer)(skBgConsumerDataPtr, skBgConsumerTaskPtr);
typedef skBgConsumerDataPtr (*skBgConsumerDataCreate)(void);
typedef void (*skBgConsumerDataDestroy)(skBgConsumerDataPtr);

typedef struct _skBgTaskNode
{
    struct _skBgTaskNode* prev;
    struct _skBgTaskNode* next;
    skBgConsumerTaskPtr task;
} skBgTaskNode;

typedef struct
{
    skBgConsumerDataPtr* consumerData;
    void* threads;
    skBgConsumer consumerFunc;
    skBgConsumerDataDestroy consumerDataDestroyer;
    int numThreads;
    int shutdown;
    void* mutex;
    void* cndVar;
    skBgTaskNode head;
} skBgTaskManager;

SK_API
/*!
 *
 */
int skBgTaskManagerInit(skBgTaskManager* mgr, int numThreads,
                        skBgConsumerDataCreate dataCreate,
                        skBgConsumerDataDestroy dataDestroy,
                        skBgConsumer consumer);

SK_API
/*!
 *
 */
void skBgTaskManagerAddTask(skBgTaskManager* mgr, skBgConsumerTaskPtr task);

SK_API
/*!
 *
 */
void skBgTaskManagerShutdown(skBgTaskManager* mgr);

SK_API
/*!
 *
 */
int skBgTaskManagerDestroy(skBgTaskManager* mgr);

#endif/*_SERVKIT_BGTASKMANAGER_H_*/
