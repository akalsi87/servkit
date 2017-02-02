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
}

static
int cleanupTaskManagerCreation(skBgTaskManager* mgr)
{
    if (mgr->consumerData) {
        int const numThreads = mgr->numThreads;
        for (int i = 0; i < numThreads; ++i) {
            mgr->consumerDataDestroyer(mgr->consumerData[i]);
        }
    }
    free(mgr->consumerData);
    free(mgr->threads);
    pthread_mutex_destroy((pthread_mutex_t*)mgr->mutex);
    free(mgr->mutex);
    pthread_cond_destroy((pthread_cond_t*)mgr->cndVar);
    free(mgr->cndVar);
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
    skBgConsumerTaskPtr taskPtr;

    for (; idx < mgr->numThreads; ++idx) {
        if (pthread_equal(threads[idx], self)) {
            break;
        }
    }
    skAssert(idx < mgr->numThreads);

    // try to pop an element
    while (1) {
        pthread_mutex_lock(mutex);
        node = firstNode->next;
        while (node == firstNode || mgr->shutdown) {
            pthread_cond_wait(cndVar, mutex);
            node = firstNode->next;
        }
        if (mgr->shutdown) {
            pthread_mutex_unlock(mutex);
            break;
        }
        firstNode->next = node->next;
        node->next->prev = firstNode;
        taskPtr = node->task;
        free(node);
        mgr->consumerFunc(mgr->consumerData[idx], taskPtr);
        pthread_mutex_unlock(mutex);
    }

    return 0;
}

int skBgTaskManagerCreate(skBgTaskManager* mgr, int numThreads,
                          skBgConsumerDataCreate dataCreate,
                          skBgConsumerDataDestroy dataDestroy,
                          skBgConsumer consumer)
{
    pthread_t* threads;
    initEmpty(mgr);
    // create consumer data
    mgr->numThreads = numThreads;
    mgr->consumerFunc = consumer;
    mgr->consumerDataDestroyer = dataDestroy;
    mgr->shutdown = 0;
    mgr->consumerData =
        (skBgConsumerDataPtr*)malloc(sizeof(skBgConsumerDataPtr) * numThreads);
    if (!mgr->consumerData) {
        return cleanupTaskManagerCreation(mgr);
    }
    for (int i = 0; i < numThreads; ++i) {
        mgr->consumerData[i] = dataCreate();
    }
    threads = malloc(sizeof(pthread_t)*numThreads);
    if (!threads) {
        return -1;
    }
    mgr->threads = threads;
    mgr->mutex = malloc(sizeof(pthread_mutex_t));
    if (!mgr->mutex) {
        return cleanupTaskManagerCreation(mgr);
    }
    if (pthread_mutex_init((pthread_mutex_t*)mgr->mutex, 0) != 0) {
        return cleanupTaskManagerCreation(mgr);
    }
    mgr->cndVar = malloc(sizeof(pthread_cond_t));
    if (!mgr->cndVar) {
        return cleanupTaskManagerCreation(mgr);
    }
    if (pthread_cond_init((pthread_cond_t*)mgr->cndVar, 0) != 0) {
        return cleanupTaskManagerCreation(mgr);
    }
    // initialize all threads
    for (int i = 0; i < numThreads; ++i) {
        int ok = pthread_create(threads + i, 0, threadRunner, mgr);
        skAssert(ok);
    }
    return 0;
}

void skBgTaskManagerAddTask(skBgTaskManager* mgr, skBgConsumerTaskPtr task)
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
    pthread_mutex_unlock(mutex);
    pthread_cond_signal(cndVar);
}

void skBgTaskManagerShutdown(skBgTaskManager* mgr)
{
    pthread_t* threads = (pthread_t*)mgr->threads;
    pthread_mutex_t* mutex = (pthread_mutex_t*)mgr->mutex;
    pthread_cond_t* cndVar = (pthread_cond_t*)mgr->cndVar;
    pthread_mutex_lock(mutex);
    mgr->shutdown = 1;
    pthread_mutex_unlock(mutex);
    pthread_cond_signal(cndVar);
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

int skBgTaskManagerDestroy(skBgTaskManager* mgr)
{
    skBgTaskManagerShutdown(mgr);
    cleanupTaskManagerCreation(mgr);
    return 0;
}
