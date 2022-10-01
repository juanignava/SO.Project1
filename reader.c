#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define NUM_ITEMS 180

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

typedef struct
{
    int memory_used;
    double blocked_sem_time;
    int total_pixels_processed;
    double total_kernel_time;
    int pixels_greater_than_175;
    int encoders_counter;
    int decoders_counter;
} Stats;


// FUNCTIONS
void read_info_manual(QueueData *queue, QueueInfo *queue_info, Stats *stats) 
{
    for (int i = 0; i < NUM_ITEMS; i++)
    {

        sem_wait(&queue_info->sem_empty);

        clock_t begin = clock();
        int val = queue[queue_info->next_output].value;
        printf("Reading value: %d\n", val);
        queue_info->next_output = (i+1) % chunk; // for circular list
        clock_t end = clock();

        // Adding reading time
        stats->total_kernel_time += (double)(end - begin) / CLOCKS_PER_SEC;

        sem_post(&queue_info->sem_filled);
        getchar(); 
    }
    
}

void read_info_auto(QueueData *queue, QueueInfo *queue_info, Stats *stats)
{
    for (int i = 0; i < NUM_ITEMS; i++)
    {
        sem_wait(&queue_info->sem_empty);

        clock_t begin = clock();
        int val = queue[queue_info->next_output].value;
        printf("Reading value: %d\n", val);
        queue_info->next_output = (i+1) % chunk; // for circular list
        clock_t end = clock();

        // Adding reading time
        stats->total_kernel_time += (double)(end - begin) / CLOCKS_PER_SEC;

        sem_post(&queue_info->sem_filled);
    }
    
}

int main(int argc, char *argv[])
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

    int fd_stats = open("/tmp/project_1_stats", O_RDWR | O_CREAT, 0644);
    if (fd_stats < 0) {
        perror("project_1_stats");
        exit(1);
    }
    ftruncate(fd_stats, sizeof(Stats));
    
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

    Stats *stats = mmap(NULL, sizeof(Stats), PROT_READ | PROT_WRITE,
        MAP_SHARED, fd_stats, 0);

    stats->decoders_counter += 1;
    
    // fill the queue with the data
    //printf("Antes del llamado de la función");
    if(strcmp(argv[1], "auto") == 0){
      read_info_auto(queue, queue_info, stats);
    }

    else if(strcmp(argv[1], "manual") == 0){
      read_info_manual(queue, queue_info, stats);
    }
    else{
        printf("Indicate a valid operation method: manual or auto");
    }
    unlink("/tmp/project_1_queue");
    unlink("/tmp/project_1_info");

    stats->decoders_counter -= 1;
    if (stats->encoders_counter == 0 && stats->decoders_counter == 0){
        int callStats = system("./stats");
    }

    unlink("/tmp/project_1_stats");
   
    close(fd_queue);
    close(fd_info);
    close(fd_stats);
    return 0;
}