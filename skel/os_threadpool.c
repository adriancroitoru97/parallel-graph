#include "os_threadpool.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/* === TASK === */

/* Creates a task that thread must execute */
os_task_t *task_create(void *arg, void (*f)(void *))
{
    os_task_t *task = malloc(sizeof(os_task_t));
    if (!task) {
        return NULL;
    }

    task->argument = arg;
    task->task = f;

    return task;
}

/* Add a new task to threadpool task queue */
void add_task_in_queue(os_threadpool_t *tp, os_task_t *t)
{
    os_task_queue_t *new_node = malloc(sizeof(os_task_queue_t));
    if (!new_node) {
        return;
    }

    new_node->task = t;
    new_node->next = NULL;

    pthread_mutex_lock(&tp->taskLock);

    if (!tp->tasks) {
        tp->tasks = new_node;
    } else {
        os_task_queue_t *curr = tp->tasks;
        while (curr && curr->next) {
            curr = curr->next;
        }
        curr->next = new_node;
    }

    pthread_mutex_unlock(&tp->taskLock);
}

/* Get the head of task queue from threadpool */
os_task_t *get_task(os_threadpool_t *tp)
{
    os_task_t *head = NULL;

    pthread_mutex_lock(&tp->taskLock);

    if (tp->tasks) {
        head = tp->tasks->task;

        /* Delete the current task from the threadpool */
        os_task_queue_t *current = tp->tasks;
        tp->tasks = tp->tasks->next;
        free(current);
    }

    pthread_mutex_unlock(&tp->taskLock);

    return head;
}

/* === THREAD POOL === */

/* Initialize the new threadpool */
os_threadpool_t *threadpool_create(unsigned int nTasks, unsigned int nThreads)
{
    os_threadpool_t *tp = malloc(sizeof(os_threadpool_t));
    if (!tp) {
        return NULL;
    }

    tp->should_stop = 0;
    tp->num_threads = nThreads;
    tp->tasks = NULL;
    pthread_mutex_init(&tp->taskLock, NULL);

    tp->threads = malloc(nThreads * sizeof(pthread_t));
    for (int i = 0; i < nThreads; i++) {
        pthread_create(&tp->threads[i], NULL, thread_loop_function, tp);
    }

    return tp;
}

/* Loop function for threads */
void *thread_loop_function(void *args)
{
    os_threadpool_t *tp = (os_threadpool_t *) args;

    while (!tp->should_stop) {
        os_task_t *current = get_task(tp);
        if (current) {
            current->task(current->argument);
            free(current);
        }
    }

    return NULL;
}

/* Stop the thread pool once a condition is met */
void threadpool_stop(os_threadpool_t *tp, int (*processingIsDone)(os_threadpool_t *))
{
    /* Threadpool waits until the processing is done,
       even if there are no running tasks */
    while (!processingIsDone(tp)) {
        ;
    }

    tp->should_stop = 1;

    for (int i = 0; i < tp->num_threads; i++) {
        pthread_join(tp->threads[i], NULL);
    }

    pthread_mutex_destroy(&tp->taskLock);
    free(tp->threads);
    free(tp);
}
