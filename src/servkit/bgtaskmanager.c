/**
 * \file bgtaskmanager.c
 * \date Feb 01, 2017
 */

#include <servkit/asserts.h>
#include <servkit/bgtaskmanager.h>

#include <pthread.h>
#include <stdlib.h>

static
void initEmpty(skBgTaskManager* mgr)
{
    // null out and initialize
    mgr->consumerData =  0;
    mgr->threads = 0;
    mgr->consumerFunc = 0;
    mgr->consumerDataDestroyer = 0;
    mgr->numThreads = 0;
    mgr->shutdown = 0;
    mgr->mutex = 0;
    mgr->cndVar = 0;
    mgr->head.prev = &(mgr->head);
    mgr->head.next = &(mgr->head);
    mgr->ctxt = 0;
    mgr->consumeRemaining = 0;
}

static
int cleanupTaskManager(skBgTaskManager* mgr)
{
    if (mgr->consumerData) {
        int const numThreads = mgr->numThreads;
        for (int i = 0; i < numThreads; ++i) {
            mgr->consumerDataDestroyer(mgr->ctxt, mgr->consumerData[i]);
        }
    }
    free(mgr->consumerData);
    free(mgr->threads);
    if (mgr->mutex) pthread_mutex_destroy((pthread_mutex_t*)mgr->mutex);
    free(mgr->mutex);
    mgr->mutex = 0;
    if (mgr->cndVar) pthread_cond_destroy((pthread_cond_t*)mgr->cndVar);
    free(mgr->cndVar);
    mgr->cndVar = 0;
    skAssert(mgr->head.next == &(mgr->head) && mgr->head.prev == mgr->head.next);
    initEmpty(mgr);
    return -1;
}

static
void* threadRunner(void* taskmgr)
{
    int idx = 0;
    pthread_t self = pthread_self();
    skBgTaskManager* mgr = (skBgTaskManager*)taskmgr;
    pthread_t* threads = (pthread_t*)mgr->threads;
    pthread_mutex_t* mutex = (pthread_mutex_t*)mgr->mutex;
    pthread_cond_t* cndVar = (pthread_cond_t*)mgr->cndVar;
    skBgTaskNode* firstNode = &(mgr->head);
    skBgTaskNode* node;
    skBgTaskPtr taskPtr;

    for (; idx < mgr->numThreads; ++idx) {
        if (pthread_equal(self, threads[idx])) {
            break;
        }
    }
    skAssert(idx < mgr->numThreads);

    // try to pop an element
    while (1) {
        pthread_mutex_lock(mutex);
        while ((node = firstNode->next) == firstNode && !mgr->shutdown) {
            pthread_cond_wait(cndVar, mutex);
        }
        if (mgr->shutdown) {
            pthread_mutex_unlock(mutex);
            break;
        }
        firstNode->next = node->next;
        node->next->prev = firstNode;
        taskPtr = node->task;
        pthread_mutex_unlock(mutex);
        free(node);
        mgr->consumerFunc(mgr->consumerData[idx], taskPtr);
    }

    if (mgr->consumeRemaining) {
        int done = 0;
        while (1) {
            pthread_mutex_lock(mutex);
            if ((node = firstNode->next) != firstNode) {
                firstNode->next = node->next;
                node->next->prev = firstNode;
                taskPtr = node->task;
            } else {
                done = 1;
            }
            pthread_mutex_unlock(mutex);
            if (done) {
                break;
            }
            free(node);
            mgr->consumerFunc(mgr->consumerData[idx], taskPtr);
        }
    }

    return 0;
}

int skBgTaskManagerInit(skBgTaskManager* mgr, int numThreads, void* ctxt,
                        skBgDataCreate dataCreate,
                        skBgDataDestroy dataDestroy,
                        skBgFunc consumer)
{
    pthread_t* threads;
    pthread_mutex_t* mutex;
    initEmpty(mgr);
    // create consumer data
    mgr->numThreads = numThreads;
    mgr->consumerFunc = consumer;
    mgr->consumerDataDestroyer = dataDestroy;
    mgr->shutdown = 0;
    mgr->consumerData =
        (skBgDataPtr*)malloc(sizeof(skBgDataPtr) * numThreads);
    if (!mgr->consumerData) {
        return cleanupTaskManager(mgr);
    }
    for (int i = 0; i < numThreads; ++i) {
        mgr->consumerData[i] = dataCreate(ctxt);
    }
    threads = malloc(sizeof(pthread_t)*numThreads);
    if (!threads) {
        return -1;
    }
    mgr->threads = threads;
    mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if (!mutex) {
        return cleanupTaskManager(mgr);
    }
    mgr->mutex = mutex;
    if (pthread_mutex_init((pthread_mutex_t*)mgr->mutex, 0) != 0) {
        return cleanupTaskManager(mgr);
    }
    mgr->cndVar = malloc(sizeof(pthread_cond_t));
    if (!mgr->cndVar) {
        return cleanupTaskManager(mgr);
    }
    if (pthread_cond_init((pthread_cond_t*)mgr->cndVar, 0) != 0) {
        return cleanupTaskManager(mgr);
    }
    // initialize all threads
    for (int i = 0; i < numThreads; ++i) {
        int fail = pthread_create(threads + i, 0, threadRunner, mgr);
        skAssert(!fail);
        (void)fail;
    }
  // waitTillDone:
  //   {
  //       int done = 0;
  //       pthread_mutex_lock(mutex);
  //       done = mgr->numThreads == 0;
  //       pthread_mutex_unlock(mutex);
  //       if (!done) {
  //           goto waitTillDone;
  //       }
  //   }
    mgr->numThreads = numThreads;
    mgr->ctxt = ctxt;
    return 0;
}

void skBgTaskManagerAddTask(skBgTaskManager* mgr, skBgTaskPtr task)
{
    pthread_mutex_t* mutex = (pthread_mutex_t*)mgr->mutex;
    pthread_cond_t* cndVar = (pthread_cond_t*)mgr->cndVar;
    skBgTaskNode* firstNode = &(mgr->head);
    skBgTaskNode* node = (skBgTaskNode*)malloc(sizeof(skBgTaskNode));
    if (!node) {
        return;
    }
    node->task = task;
    pthread_mutex_lock(mutex);
    node->next = firstNode;
    node->prev = firstNode->prev;
    firstNode->prev = node;
    node->prev->next = node;
    pthread_cond_signal(cndVar);
    pthread_mutex_unlock(mutex);
}

static
void taskManagerShutdown(skBgTaskManager* mgr, int consumeRemainingTasks)
{
    pthread_t* threads = (pthread_t*)mgr->threads;
    pthread_mutex_t* mutex = (pthread_mutex_t*)mgr->mutex;
    pthread_cond_t* cndVar = (pthread_cond_t*)mgr->cndVar;
    pthread_mutex_lock(mutex);
    mgr->shutdown = 1;
    mgr->consumeRemaining = consumeRemainingTasks;
    pthread_cond_broadcast(cndVar);
    pthread_mutex_unlock(mutex);
    if (threads) {
        for (int i = 0; i < mgr->numThreads; ++i) {
            void* retval;
            pthread_join(threads[i], &retval);
            skAssert(retval == 0);
        }
        free(threads);
        mgr->threads = 0;
    }
}

int skBgTaskManagerDestroy(skBgTaskManager* mgr, int finishTasks)
{
    taskManagerShutdown(mgr, finishTasks);
    cleanupTaskManager(mgr);
    return 0;
}
