#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdio.h>

typedef void *(*TPCallback)(void *args);

typedef struct Task {
    TPCallback function;
    void *arg;
    struct Task *next;

} Task;

typedef struct ThreadPool {
    pthread_t *threads;
    int count;
    
    // Task queue
    Task *queue_head;
    Task *queue_tail;
    
    // Synchronization
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    
    // Control
    bool shutdown;
} ThreadPool;

int thread_pool_init(ThreadPool *out_pool);
int thread_pool_run(const ThreadPool *pool, TPCallback callback, void *arg);
int thread_pool_enqueue(ThreadPool *pool, TPCallback callback, void *arg);
int thread_pool_destroy(ThreadPool *pool);

#endif