#include "include/thread_pool.h"
#include <pthread.h>

static void *worker_thread(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;
    
    while (true) {
        pthread_mutex_lock(&pool->lock);
        while (!pool->task_head && !pool->stop) {
            pthread_cond_wait(&pool->cond, &pool->lock);
        }

        if (pool->stop && !pool->task_head) {
            pthread_mutex_unlock(&pool->lock);
            break;
        }

        Task *task = pool->task_head;
        pool->task_head = task->next;
        if (!pool->task_head) {
            pool->task_tail = NULL;
        }

        pthread_mutex_unlock(&pool->lock);

        task->func(task->arg);
        free(task);
    }

    return NULL;
}

static int optimized_thread_count() {
    int count = sysconf(_SC_NPROCESSORS_ONLN);
    if (count < 1) {
        fprintf(stderr, "Failed to get the number of processors, defaulting to 1 thread\n");
        return 1; // default to 1 thread if sysconf fails
    }
    return count;
}

ThreadPool *create_thread_pool() {
    ThreadPool *pool = malloc(sizeof(ThreadPool));
    if (!pool) {
        fprintf(stderr, "Failed to allocate memory for thread pool\n");
        return NULL;
    }

    pool->thread_count = optimized_thread_count();
    pool->threads = malloc(pool->thread_count * sizeof(pthread_t));
    if (!pool->threads) {
        fprintf(stderr, "Failed to allocate memory for thread pool threads\n");
        free(pool);
        return NULL;    
    }  
    pool->task_head = pool->task_tail = NULL;
    pool->stop = false;

    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->cond, NULL);

    for (int i = 0; i < pool->thread_count; i++) {
        if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            for (int j = 0; j < i; j++) {
                pthread_cancel(pool->threads[j]);
            }
            free(pool->threads);
            free(pool);
            return NULL;
        }
    }

    return pool;
}

int thread_pool_enqueue(ThreadPool *pool, task_func_t func, void *arg) {
    Task *task = malloc(sizeof(Task));
    if (!task) {
        fprintf(stderr, "Failed to allocate memory for task\n");
        return 1;
    }

    task->func = func;
    task->arg = arg;
    task->next = NULL;

    pthread_mutex_lock(&pool->lock);

    if (pool->task_tail) {
        pool->task_tail->next = task;
    } else {
        pool->task_head = task;
    }

    pool->task_tail = task;

    pthread_cond_signal(&pool->cond);
    pthread_mutex_unlock(&pool->lock);

    return 0;
}

void thread_pool_destroy(ThreadPool *pool) {
    if (!pool) return;

    pthread_mutex_lock(&pool->lock);
    pool->stop = true;
    pthread_cond_broadcast(&pool->cond);
    pthread_mutex_unlock(&pool->lock);

    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    free(pool->threads);

    Task *task = pool->task_head;
    while (task) {
        Task *next = task->next;
        free(task);
        task = next;
    }

    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->cond);
    free(pool);
}