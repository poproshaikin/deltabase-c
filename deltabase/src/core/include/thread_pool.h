#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

typedef void (*task_func_t)(void *);

typedef struct Task {
    task_func_t func;
    void *arg;
    struct Task *next;
} Task;

typedef struct ThreadPool {
    pthread_t *threads;
    int thread_count;

    Task *task_head;
    Task *task_tail;

    pthread_mutex_t lock;
    pthread_cond_t cond;
    bool stop;
} ThreadPool;

ThreadPool *thread_pool_create(int num_threads);
int thread_pool_enqueue(ThreadPool *pool, task_func_t func, void *arg);
void thread_pool_destroy(ThreadPool *pool);

#endif 
