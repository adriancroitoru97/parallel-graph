#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include "os_graph.h"
#include "os_threadpool.h"
#include "os_list.h"

#define MAX_TASK 100
#define MAX_THREAD 4

int sum = 0;
os_graph_t *graph;


/* Variable used to count the number of visited nodes */
int nr_visited = 0;

/* Struct used to store the arguments implied by the task */
typedef struct {
    os_threadpool_t *tp;
    pthread_mutex_t *sum_lock;
    pthread_mutex_t *visited_locks;
    int index;
} process_node_t;

/* Method used to determine if the graph was completely traversed */
int processing_is_done(os_threadpool_t *tp) {
    return nr_visited == graph->nCount ? 1 : 0;
}

void process_node(void *args)
{
    /* Take the given struct argument, store it locally and free the memory */
    process_node_t *tmp = (process_node_t*) args;
    os_threadpool_t *tp = tmp->tp;
    pthread_mutex_t *sum_lock = tmp->sum_lock;
    pthread_mutex_t *visited_locks = tmp->visited_locks;
    int index = tmp->index;
    free(tmp);

    /* The actual graph node */
    os_node_t *node = graph->nodes[index];

    /* Add the current node to the sum */
    pthread_mutex_lock(sum_lock);
    sum += node->nodeInfo;
    nr_visited++;
    pthread_mutex_unlock(sum_lock);

    /* Create new tasks for the neighbours of the current node */
    for (int i = 0; i < node->cNeighbours; i++) {
        pthread_mutex_lock(&visited_locks[node->neighbours[i]]);
        if (!graph->visited[node->neighbours[i]]) {
            graph->visited[node->neighbours[i]] = 1;

            /* Create the data structure used by the task */
            process_node_t *tmp = (process_node_t*)malloc(sizeof(process_node_t));
            tmp->tp = tp;
            tmp->sum_lock = sum_lock;
            tmp->index = node->neighbours[i];
            tmp->visited_locks = visited_locks;

            os_task_t *task = task_create((void*)tmp, process_node);
            add_task_in_queue(tp, task);
        }
        pthread_mutex_unlock(&visited_locks[node->neighbours[i]]);
    }
}


int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: ./main input_file\n");
        exit(1);
    }

    FILE *input_file = fopen(argv[1], "r");

    if (input_file == NULL) {
        printf("[Error] Can't open file\n");
        return -1;
    }

    graph = create_graph_from_file(input_file);
    if (graph == NULL) {
        printf("[Error] Can't read the graph from file\n");
        return -1;
    }

    /* --------------------------- Thread Pool Aproach --------------------------- */

    os_threadpool_t *tp = threadpool_create(MAX_TASK, MAX_THREAD);

    /* Lock used to guarantee that no more than one thread
       modifies the `sum` variable at a time */
    pthread_mutex_t sum_lock;
    pthread_mutex_init(&sum_lock, NULL);

    /* Array of locks used to guarantee that no more than one thread
       is accessing the graph->visited[] array for a given index */
    pthread_mutex_t *visited_locks = (pthread_mutex_t*) malloc(graph->nCount * sizeof(pthread_mutex_t));
    for (int i = 0; i < graph->nCount; i++) {
        pthread_mutex_init(&visited_locks[i], NULL);
    }

    /* Process the nodes of the graph */
    for (int i = 0; i < graph->nCount; i++)
    {
        pthread_mutex_lock(&visited_locks[i]);
        if (!graph->visited[i]) {
            graph->visited[i] = 1;

            /* Create the data structure used by the task */
            process_node_t *tmp = (process_node_t*)malloc(sizeof(process_node_t));
            tmp->tp = tp;
            tmp->sum_lock = &sum_lock;
            tmp->index = i;
            tmp->visited_locks = visited_locks;

            os_task_t *task = task_create((void*)tmp, process_node);
            add_task_in_queue(tp, task);
        }
        pthread_mutex_unlock(&visited_locks[i]);
    }

    threadpool_stop(tp, processing_is_done);

    /* --------------------------- Thread Pool Aproach Done --------------------------- */


    printf("%d", sum);
    return 0;
}
