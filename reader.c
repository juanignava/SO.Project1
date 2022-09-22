#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>

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
    sem_t sem_filled;
    sem_t sem_empty;
    int next_input;
    int next_output;
} QueueInfo;


// FUNCTIONS
void read_info(QueueData *queue, QueueInfo *queue_info)
{
    for (int i = 0; i < NUM_ITEMS; i++)
    {
        sem_wait(&queue_info->sem_empty);
        int val = queue[queue_info->next_output].value;
        printf("Reading value: %d\n", val);
        queue_info->next_output = (i+1) % chunk; // for circular list
        sem_post(&queue_info->sem_filled);
    }
    
}

int main()
{
    // opens the file descriptor that has to be mapped to the
    //     shared memory
    int fd_queue = open("/tmp/project_1_queue", O_RDWR | O_CREAT, 0644);
    if (fd_queue < 0) {
        perror("project_1_queue");
        exit(1);
    }
    ftruncate(fd_queue, chunk*sizeof(QueueData));
    
    int fd_info = open("/tmp/project_1_info", O_RDWR | O_CREAT, 0644);
    if (fd_info < 0) {
        perror("project_1_info");
        exit(1);
    }
    ftruncate(fd_info, sizeof(QueueInfo));
    
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


    // fill the queue with the data
    //printf("Antes del llamado de la función");
    read_info(queue, queue_info);

    unlink("/tmp/project_1_queue");
    unlink("/tmp/project_1_info"); 
   
    close(fd_queue);
    close(fd_info);
    return 0;
}