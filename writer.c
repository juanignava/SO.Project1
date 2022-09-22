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

#define NUM_ITEMS 120

// GLOBALS

int chunk = 10;

// STRUCT
typedef struct
{
    int value;
} QueueData;

typedef struct
{
    sem_t *sem_filled;
    sem_t *sem_empty;
    int next_input;
    int next_output;
} QueueInfo;


// FUNCTIONS
void *write_info(QueueData *queue, QueueInfo *queue_info)
{
    for (int i = 0; i < NUM_ITEMS; i++)
    {
        sem_wait(queue_info->sem_filled);
        queue[queue_info->next_input].value = i;
        queue_info->next_input = (i+1) % chunk; // for circular list
        sem_post(queue_info->sem_empty);
    }
    
}

int main()
{
    // opens the file descriptor that has to be mapped to the
    //     shared memory
    int fd_queue = open("/tmp/project_1_queue", O_RDWR | O_CREAT, 0644);
    int fd_info = open("/tmp/project_1_info", O_RDWR | O_CREAT, 0644);
    //ftruncate(fd, N*sizeof(int));

    // memory mapping function instance
    //      address: NULL means the kernel can place the mapping anywhere it sees fit
    //      lenght: number of bytes to be mapped, the array will contain 10 integers
    //      protect: PROT_READ | PROT_WRITE the access allows reading and writing on the content
    //      flags: MAP_SHARED this flag is used to share the mapping with all other processes,
    //          which are mapped to this object
    //      filedes: file descriptor to be mapped
    //      offset: 0, offset from where the mapping started

    // source: https://linuxhint.com/using_mmap_function_linux/
    QueueData *queue =  mmap(NULL, chunk*sizeof(QueueData), PROT_READ | PROT_WRITE,
        MAP_SHARED, fd_queue, 0);
    
    
    QueueInfo *queue_info = mmap(NULL, sizeof(QueueInfo), PROT_READ | PROT_WRITE,
        MAP_SHARED, fd_info, 0);
    
    /*
    // sem_filled begins in 0 because everuthing is empty
    sem_t *sem_filled = malloc(sizeof(sem_t));
    sem_init(sem_filled, 1, 0); 
    // sem_empty begins in {chunk} because everuthing is empty
    sem_t *sem_empty = malloc(sizeof(sem_t));
    sem_init(sem_empty, 1, chunk);
    int next_input = 0;
    int next_output = 0;
    */

    // sem_filled begins in 0 because everuthing is empty
    //sem_init(queue_info->sem_filled, 1, 0); 
    // sem_empty begins in {chunk} because everuthing is empty
    //sem_init(queue_info->sem_empty, 1, chunk);
    //queue_info->next_input = 0;
    //queue_info->next_output = 0;

    /*
    queue_info->sem_filled = sem_filled;
    queue_info->sem_empty = sem_empty;
    queue_info->next_input = 0;
    queue_info->next_output = 0;
    */
    

    // fill the queue with the data
    //write_info(queue, queue_info);
   

    close(fd_queue);
    close(fd_info);

    return 0;
}