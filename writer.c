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

#define NUM_ITEMS 100

// GLOBALS

int chunk = 10;

// STRUCT
typedef struct
{
    int value;
    int id;
} QueueData;


typedef struct
{
    sem_t sem_filled;
    sem_t sem_empty;
    int next_input;
    int next_output;
    int amount_encoders;
    int amount_decoders;
} QueueInfo;


// FUNCTIONS
void write_info_auto(QueueData *queue, QueueInfo *queue_info, int image_id)
{
    for (int i = 0; i < NUM_ITEMS; i++)
    {
        sem_wait(&queue_info->sem_filled);
        printf("Adding value: %d in image: %d\n", i, image_id);
        queue[queue_info->next_input].value = i;
        queue[queue_info->next_input].id = image_id;
        queue_info->next_input = (i+1) % chunk; // for circular list
        sem_post(&queue_info->sem_empty);
    }
    
}

void write_info_manual(QueueData *queue, QueueInfo *queue_info, int image_id)
{
    for (int i = 0; i < NUM_ITEMS; i++)
    {
        sem_wait(&queue_info->sem_filled);
        printf("Adding value: %d in image: %d\n", i, image_id);
        queue[queue_info->next_input].value = i;
        queue[queue_info->next_input].id = image_id;
        queue_info->next_input = (i+1) % chunk; // for circular list
        sem_post(&queue_info->sem_empty);
        getchar(); 
    }
    
}


int main(int argc, char *argv[])
{

    // determine if the file descriptor already exists
    //   if it does, then the shared memory has been created
    int file_exists = access("/tmp/project_1_queue", F_OK) == 0 ? 1 : 0;
    
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

    int image_id;

    // if the file exists
    if(!file_exists){

        // sem_filled begins in chunk because everuthing is empty
        sem_t sem_filled;
        sem_init(&sem_filled, 1, chunk); 

        // sem_empty begins in 0 because everuthing is empty
        sem_t sem_empty;
        sem_init(&sem_empty, 1, 0);

        // input and output memory pointers initial values
        int next_input = 0;
        int next_output = 0;

        // queue_info initial data
        queue_info->sem_filled = sem_filled;
        queue_info->sem_empty = sem_empty;
        queue_info->next_input = 0;
        queue_info->next_output = 0;
        queue_info->amount_encoders = 1;
        queue_info->amount_decoders = 0;

        // define image id for writing
        image_id = queue_info->amount_encoders;
    }
    else {
        queue_info->amount_encoders += 1;
        // define image id for writing
        image_id = queue_info->amount_encoders;
    }
    
    if(strcmp(argv[1], "auto") == 0){
      write_info_auto(queue, queue_info, image_id);
    }

    else if(strcmp(argv[1], "manual") == 0){
      write_info_manual(queue, queue_info, image_id);
    }
    else{
        printf("Indicate a valid operation method: manual or auto");
    }
    
    close(fd_queue);
    close(fd_info);
    return 0;
}