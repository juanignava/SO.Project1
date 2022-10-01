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
void read_info_manual(QueueData *queue, QueueInfo *queue_info, int image_id)
{
    for (int i = 0; i < NUM_ITEMS; i++)
    {
        sem_wait(&queue_info->sem_empty);
        if (queue[queue_info->next_output].id == image_id){
            int val = queue[queue_info->next_output].value;
            int read_image_id = queue[queue_info->next_output].id;
            printf("Reading value: %d from image: %d\n", val, read_image_id);
            queue_info->next_output = (i+1) % chunk; // for circular list
            sem_post(&queue_info->sem_filled);
        }
        else{
            sem_post(&queue_info->sem_empty);
        }
        getchar(); 
    }
    
}

void read_info_auto(QueueData *queue, QueueInfo *queue_info, int image_id)
{
    for (int i = 0; i < NUM_ITEMS; i++)
    {
        sem_wait(&queue_info->sem_empty);
        if (queue[queue_info->next_output].id == image_id){
            int val = queue[queue_info->next_output].value;
            int read_image_id = queue[queue_info->next_output].id;
            printf("Reading value: %d from image: %d\n", val, read_image_id);
            sem_post(&queue_info->sem_filled);
        }
        else{
            sem_post(&queue_info->sem_empty);
        }
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

    // define image id to read
    queue_info->amount_decoders += 1;
    int image_id = queue_info->amount_decoders;
    printf("This is encoder: %d\n", image_id);

    // fill the queue with the data
    //printf("Antes del llamado de la función");
    if(strcmp(argv[1], "auto") == 0){
      read_info_auto(queue, queue_info, image_id);
    }

    else if(strcmp(argv[1], "manual") == 0){
      read_info_manual(queue, queue_info, image_id);
    }
    else{
        printf("Indicate a valid operation method: manual or auto");
    }
    unlink("/tmp/project_1_queue");
    unlink("/tmp/project_1_info"); 
   
    close(fd_queue);
    close(fd_info);
    return 0;
}