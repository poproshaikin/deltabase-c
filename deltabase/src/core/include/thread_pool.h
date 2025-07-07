//
// Created by poproshaikin on 8.7.25.
//

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdatomic.h>

typedef struct TaskHandle {
    void (*task_function)(void *arg);
    void *arg;
    int is_completed;
    pthread_mutex_t is_completed_mutex;
    pthread_t thread_id;
} TaskHandle;

typedef struct ThreadPool {
    int recommended_thread_count;
    int thread_count;
    pthread_t *threads;
    TaskHandle **tasks;
    int task_count;
} ThreadPool;

ThreadPool *create_thread_pool();

void *run_task(void (*task_function)(void *arg), void *arg, ThreadPool *pool);

#endif //THREAD_POOL_H
