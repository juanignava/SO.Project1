#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include "utils.h"
#include "sem.h"

#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>

// STRUCTS

typedef struct {
  int *array;
  int length;
  int next_in;
  int next_out;
  Semaphore *mutex;
  Semaphore *items;
  Semaphore *spaces;
} Queue;

typedef struct {
  Queue *queue;
} Shared;

// GLOBAL

// number of elements in the array
int N = 128;

// FUNCTIONS
Queue *make_queue(int length)
{
  Queue *queue = (Queue *) malloc(sizeof(Queue));
  queue->length = length;
  queue->array = (int *) malloc(length * sizeof(int));
  queue->next_in = 0;
  queue->next_out = 0;
  queue->mutex = make_semaphore(1);
  queue->items = make_semaphore(0);
  queue->spaces = make_semaphore(length-1);
  return queue;
}

int queue_incr(Queue *queue, int i)
{
  return (i+1) % queue->length;
}

int queue_empty(Queue *queue)
{
  // queue is empty if next_in and next_out are the same
  int res = (queue->next_in == queue->next_out);
  return res;
}

int queue_full(Queue *queue)
{
  // queue is full if incrementing next_in lands on next_out
  int res = (queue_incr(queue, queue->next_in) == queue->next_out);
  return res;
}

void queue_push(Queue *queue, int item) {
  semaphore_wait(queue->spaces);
  semaphore_wait(queue->mutex);

  queue->array[queue->next_in] = item;
  queue->next_in = queue_incr(queue, queue->next_in);

  semaphore_signal(queue->mutex);
  semaphore_signal(queue->items);
}

int queue_pop(Queue *queue) {
  semaphore_wait(queue->items);
  semaphore_wait(queue->mutex);
  
  int item = queue->array[queue->next_out];
  queue->next_out = queue_incr(queue, queue->next_out);

  semaphore_signal(queue->mutex);
  semaphore_signal(queue->spaces);

  return item;
}



void *reader_entry(void *arg)
{
  int i;
  int item;
  Shared *shared = (Shared *) arg;

  for (i=0; i<N; i++) {
    printf("Before pop\n");
    item = queue_pop(shared->queue);
    printf("consuming item %d\n", item);
  }
  //pthread_exit(NULL);
}

int
main()
{
    /*
    // open the file descriptor, show an error if it does not exist
    int fd = open("/tmp/project_1",O_RDWR,0644);
    if (fd < 0) {
        perror("project_1");
        exit(1);
    }

    // memory mapping function instance
    //      address: NULL means the kernel can place the mapping anywhere it sees fit
    //      lenght: number of bytes to be mapped, the array will contain 10 integers
    //      protect: PROT_READ | PROT_WRITE the access allows reading and writing on the content
    //      flags: MAP_SHARED this flag is used to share the mapping with all other processes,
    //          which are mapped to this object
    //      filedes: file descriptor to be mapped
    //      offset: 0, offset from where the mapping started

    // source: https://linuxhint.com/using_mmap_function_linux/
    
    Shared *shared = mmap(NULL, sizeof(Shared)+sizeof(Queue)+10*sizeof(int), PROT_READ | PROT_WRITE,
        MAP_SHARED, fd, 0);
    //= mmap(NULL, sizeof(Shared), PROT_READ | PROT_WRITE,
      //  MAP_SHARED, fd, 0);
    
    //shared = make_shared();

    reader_entry(shared);

    // close the file descriptor
    close(fd);

    // stop the shared memory
    */
    unlink("/tmp/project_1_queue");
    unlink("/tmp/project_1_info");

    return 0;
}