#include "include/thread_pool.h"

static int optimized_thread_count() {
    int count = sysconf(_SC_NPROCESSORS_ONLN);
    if (count < 1) {
        fprintf(stderr, "Failed to get the number of processors, defaulting to 1 thread\n");
        return 1; 
    }
    return count;
}

static void *thread_pool_worker(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;
    
    while (true) {
        pthread_mutex_lock(&pool->queue_mutex);
        
        // Wait for a task or shutdown signal
        while (pool->queue_head == NULL && !pool->shutdown) {
            pthread_cond_wait(&pool->queue_cond, &pool->queue_mutex);
        }
        
        // If shutdown and no tasks, exit
        if (pool->shutdown && pool->queue_head == NULL) {
            pthread_mutex_unlock(&pool->queue_mutex);
            break;
        }
        
        // Get a task from the queue
        Task *task = pool->queue_head;
        if (task != NULL) {
            pool->queue_head = task->next;
            if (pool->queue_head == NULL) {
                pool->queue_tail = NULL;
            }
        }
        
        pthread_mutex_unlock(&pool->queue_mutex);
        
        // Execute the task
        if (task != NULL) {
            task->function(task->arg);
            free(task);
        }
    }
    
    return NULL;
}

int thread_pool_init(ThreadPool *out_pool) {
    int thread_count = optimized_thread_count();

    // Initialize the thread pool structure
    out_pool->threads = malloc(thread_count * sizeof(pthread_t));
    if (!out_pool->threads) {
        return 1;
    }

    out_pool->count = thread_count;
    out_pool->queue_head = NULL;
    out_pool->queue_tail = NULL;
    out_pool->shutdown = false;

    // Initialize synchronization primitives
    if (pthread_mutex_init(&out_pool->queue_mutex, NULL) != 0) {
        free(out_pool->threads);
        return 1;
    }

    if (pthread_cond_init(&out_pool->queue_cond, NULL) != 0) {
        pthread_mutex_destroy(&out_pool->queue_mutex);
        free(out_pool->threads);
        return 1;
    }

    // Create worker threads
    for (int i = 0; i < thread_count; i++) {
        if (pthread_create(&out_pool->threads[i], NULL, thread_pool_worker, out_pool) != 0) {
            // Cleanup on error
            out_pool->shutdown = true;
            pthread_cond_broadcast(&out_pool->queue_cond);
            
            // Wait for created threads to finish
            for (int j = 0; j < i; j++) {
                pthread_join(out_pool->threads[j], NULL);
            }
            
            pthread_cond_destroy(&out_pool->queue_cond);
            pthread_mutex_destroy(&out_pool->queue_mutex);
            free(out_pool->threads);
            return 1;
        }
    }

    return 0;
}

int thread_pool_enqueue(ThreadPool *pool, TPCallback callback, void *arg) {
    if (!pool || !callback) {
        return 1;
    }

    // Create a new task
    Task *task = malloc(sizeof(Task));
    if (!task) {
        return 1;
    }

    task->function = callback;
    task->arg = arg;
    task->next = NULL;

    // Add task to queue
    pthread_mutex_lock(&pool->queue_mutex);
    
    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->queue_mutex);
        free(task);
        return 1;
    }
    
    if (pool->queue_tail == NULL) {
        // Queue is empty
        pool->queue_head = task;
        pool->queue_tail = task;
    } else {
        // Add to end of queue
        pool->queue_tail->next = task;
        pool->queue_tail = task;
    }

    // Signal a waiting worker thread
    pthread_cond_signal(&pool->queue_cond);
    pthread_mutex_unlock(&pool->queue_mutex);

    return 0;
}

int thread_pool_destroy(ThreadPool *pool) {
    if (!pool) {
        return 1;
    }

    // Signal shutdown
    pthread_mutex_lock(&pool->queue_mutex);
    pool->shutdown = true;
    pthread_cond_broadcast(&pool->queue_cond);
    pthread_mutex_unlock(&pool->queue_mutex);

    // Wait for all threads to finish
    for (int i = 0; i < pool->count; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    // Clean up remaining tasks in queue
    while (pool->queue_head != NULL) {
        Task *task = pool->queue_head;
        pool->queue_head = task->next;
        free(task);
    }

    // Destroy synchronization primitives
    pthread_cond_destroy(&pool->queue_cond);
    pthread_mutex_destroy(&pool->queue_mutex);

    // Free thread array
    free(pool->threads);
    
    return 0;
}