# Parallel Graph - Thread Pool Approach

Parallel Graph computation - finding the total sum of all nodes.\
Program written in `C` language, manually implemented a `Thread Pool`.


## Thread Pool

The thread pool is implemented using a `mutex`, in order to be **thread-safe**
when adding, removing or querying elements from the `task's queue`.

All threads are created once the thread pool is created, and they forever wait
to receive tasks from the `queue` (which is implemented as a simple linked list).
Even if the user sends the thread pool `stop` command, it will make sure that all
the processing is done before actually joining the threads.


## Graph computation

The main thread creates a task for each unvisited node
(this approach makes it sure that all nodes are used in the computation).

The `process_node()` method will be the actual task executed by the thread pool
and its mission is to add the current's node value to the total sum, as well as
to create new tasks for the current's node neighbors.

There are 2 mutexes used:
* `sum_lock` is used to make sure that there are no 2 threads which
overwrite the `sum` variable concurrently
* 'visited_locks` is an array of mutexes, which is used to make sure that
there are no 2 threads trying to access `graph->visited[i]` variable at the same time

## Documentation

* https://www.geeksforgeeks.org
* https://curs.upb.ro/2022/mod/forum/view.php?id=178296
* https://www.ibm.com/docs/en/i/7.3?topic=ssw_ibm_i_73/apis/users_61.html

## License

[Adrian-Valeriu Croitoru, 332CA](https://github.com/adriancroitoru97)