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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

#define MAX_IMAGES 50

// GLOBALS
int file_exists;

// STRUCT

// struct for the shared memory blocks
typedef struct
{
    unsigned char value;
    int id;
    time_t raw_pixel_time;
    sem_t sem_write_resource;
} QueueData;

// struct for the Queue general data
typedef struct
{
    sem_t sem_write;
    sem_t sem_read;
    sem_t sem_filled;
    sem_t sem_empty;
    int next_input;
    int next_output;
    int chunk_size;
} QueueInfo;

// struct for the extra information of each image
typedef struct
{
    int id;
    int width;
    int height;
    int channels;
} ImageData;

// struct with the stats
typedef struct
{
    unsigned long real_memory_used;
    unsigned long virtual_memory_used;
    double blocked_sem_time;
    int total_pixels_processed;
    double total_kernel_time;
    int pixels_greater_than_175;
    int encoders_counter;
    int decoders_counter;
    int total_enco;
    int total_deco;
} Stats;

/**
 * This function passes an int into a binary for the key
 */
int getDecimal(int clave)
{
    int num = clave;
    int dec_value = 0;
 
    // Initializing base value to 1, i.e 2^0
    int base = 1;
 
    int temp = num;
    while (temp) {
        int last_digit = temp % 10;
        temp = temp / 10;
 
        dec_value += last_digit * base;
 
        base = base * 2;
    }
 
    return dec_value;
}

// FUNCTIONS

/**
 * Write info to the shared memory
 */
void write_info(QueueData *queue, QueueInfo *queue_info, Stats *stats, ImageData *images, char* imageName, int* chunkSize, char* mode, int* key, int id)
{
    stats->total_enco ++;
    // load image and important data for the analysis
    int width, height, channels;
    unsigned char *img = stbi_load(imageName, &width, &height, &channels, 0);
    int clave = key;
    int chunk = chunkSize;

    // save the image information
    images[id].id = id;
    images[id].width = width;
    images[id].height = height;
    images[id].channels = channels;

    for (int i = 0; i < width * height * channels; i++)
    {
        // calculate wait time and down semaphores
        clock_t begin_sem = clock();
        sem_wait(&queue_info->sem_empty);
        sem_wait(&queue_info->sem_write);
        clock_t end_sem = clock();

        time_t rawtime;
        queue->raw_pixel_time = rawtime;
        
        // add the value to shared memory
        printf("Adding value: %d in position: %d\n", img[i] ^ getDecimal(clave), i);
        int timeInfo = getTime(queue->raw_pixel_time);

        // begin kernel time
        clock_t begin = clock();
        unsigned char encoded = img[i] ^ getDecimal(clave);
        
        queue[queue_info->next_input].value = encoded;
        queue[queue_info->next_input].id = id;
        
        stats->total_pixels_processed += 1;       // Adding 1 to total pixels encoded (for stats)
        clock_t end = clock();

        stats->total_kernel_time += (double)(end - begin) / CLOCKS_PER_SEC;        // Adding kernel time (for stats)
        stats->blocked_sem_time += (double)(end_sem - begin_sem) / CLOCKS_PER_SEC; // Adding blocked sem time to stats

        // calculate pixels over 175
        if ((int)img[i] >= 176)
        {
            stats->pixels_greater_than_175 += 1; // Adding 1 to pixels greater than 175 (for stats)
        }

        // wait for an enter hit when mode is manual
        if (strcmp(mode, "manual") == 0) getchar();

        queue_info->next_input = (i + 1) % chunk; // for circular list
        sem_post(&queue_info->sem_write);
        sem_post(&queue_info->sem_filled);
        
    }
}

/*
 * Measures the current (and peak) resident and virtual memory
 * usage of your linux C process, in bytes, accurate to nearest kB.
 * Returns a 0 if memory info access was successful, else prints
 * an error message and returns 1
 */
int getMemory(unsigned long *currRealMem, unsigned long *peakRealMem, unsigned long *currVirtMem, unsigned long *peakVirtMem, Stats *stats)
{

    // stores each word in status file
    char buffer[1024] = "";

    // linux file contains this-process info
    FILE *file = NULL;
    file = fopen("/proc/self/status", "r");

    if (file == NULL)
    {
        printf("Call to getMemory FAILED; "
               "linux file proc/self/status not found!\n");
        return 1;
    }

    // read the entire file, recording mems in kB
    while (fscanf(file, " %1023s", buffer) == 1)
    {

        if (strcmp(buffer, "VmRSS:") == 0)
        {
            fscanf(file, " %lu", currRealMem);
        }
        if (strcmp(buffer, "VmHWM:") == 0)
        {
            fscanf(file, " %lu", peakRealMem);
        }
        if (strcmp(buffer, "VmSize:") == 0)
        {
            fscanf(file, " %lu", currVirtMem);
        }
        if (strcmp(buffer, "VmPeak:") == 0)
        {
            fscanf(file, " %lu", peakVirtMem);
        }
    }
    fclose(file);

    // convert kB to bytes
    unsigned int factor = 1000;
    *currRealMem *= factor;
    stats->real_memory_used += *currRealMem;
    *peakRealMem *= factor;
    *currVirtMem *= factor;
    stats->virtual_memory_used += *currVirtMem;
    *peakVirtMem *= factor;
}

/**
 * Get the time information
 */
int getTime(time_t rawtime){
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    //printf ( "Current raw time 2: %ld\n", rawtime);
    printf ( "Pixel current local time and date: %s\n", asctime (timeinfo) );

}

int main(int argc, char *argv[])
{
    // Get arguments from command line
    char imageName[50];
    strcpy(imageName, "");
    int chunkSize = -1;
    char mode[50];
    strcpy(mode, "");
    int key = -1;
    // Se leen los argumentos
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0) {
            strcpy(imageName, argv[i + 1]);
        }
        if (strcmp(argv[i], "-n") == 0) {
            chunkSize = atoi(argv[i + 1]);
        }
        if (strcmp(argv[i], "-m") == 0) {
            strcpy(mode, argv[i + 1]);
        }
        if (strcmp(argv[i], "-k") == 0) {
            key = atoi(argv[i + 1]);
        }   
    }

    // validate arguments
    if (strcmp(imageName, "") == 0 || chunkSize < 1 || strcmp(mode, "") == 0 || key < 1) {
        printf("Can not identify the image, chunk or the key\n");
        return 1;
    }
  
    // determine if the file descriptor already exists
    //   if it does, then the shared memory has been created
    file_exists = access("/tmp/project_1_queue", F_OK) == 0 ? 1 : 0;

    // opens the file descriptor that has to be mapped to the
    //     shared memory
    int fd_queue = open("/tmp/project_1_queue", O_RDWR | O_CREAT, 0644);
    if (fd_queue < 0)
    {
        perror("project_1_queue");
        exit(1);
    }
    ftruncate(fd_queue, chunkSize * sizeof(QueueData));

    // file descriptor for the queues shared data
    int fd_info = open("/tmp/project_1_info", O_RDWR | O_CREAT, 0644);
    if (fd_info < 0)
    {
        perror("project_1_info");
        exit(1);
    }
    ftruncate(fd_info, sizeof(QueueInfo));

    // file descriptor for the queues stats
    int fd_stats = open("/tmp/project_1_stats", O_RDWR | O_CREAT, 0644);
    if (fd_stats < 0)
    {
        perror("project_1_stats");
        exit(1);
    }
    ftruncate(fd_stats, sizeof(Stats));

    // file descroptor for the images
    int fd_images = open("/tmp/project_1_images", O_RDWR | O_CREAT, 0644);
    if (fd_stats < 0)
    {
        perror("project_1_images");
        exit(1);
    }
    ftruncate(fd_images, MAX_IMAGES * sizeof(ImageData));

    // memory mapping function instance
    //      address: NULL means the kernel can place the mapping anywhere it sees fit
    //      lenght: number of bytes to be mapped, the array will contain 10 integers
    //      protect: PROT_READ | PROT_WRITE the access allows reading and writing on the content
    //      flags: MAP_SHARED this flag is used to share the mapping with all other processes,
    //          which are mapped to this object
    //      filedes: file descriptor to be mapped
    //      offset: 0, offset from where the mapping started

    // source: https://linuxhint.com/using_mmap_function_linux/
    QueueInfo *queue_info = mmap(NULL, sizeof(QueueInfo), PROT_READ | PROT_WRITE,
                                 MAP_SHARED, fd_info, 0);

    Stats *stats = mmap(NULL, sizeof(Stats), PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd_stats, 0);

    ImageData *images = mmap(NULL, MAX_IMAGES*sizeof(ImageData), PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd_images, 0);

    // if the file does not exist init the important data
    if (!file_exists)
    {
        // sem_filled begins in 0 because everything is empty
        sem_t sem_filled;
        sem_init(&sem_filled, 1, 0);
        // sem_empty begins in chunk because everuthing is empty
        sem_t sem_empty;
        sem_init(&sem_empty, 1, chunkSize);
        int next_input = 0;
        int next_output = 0;

        // init queue info shared data
        queue_info->sem_filled = sem_filled;
        queue_info->sem_empty = sem_empty;
        queue_info->next_input = 0;
        queue_info->next_output = 0;
        queue_info->chunk_size = chunkSize;

        int total_enco = 0;

        sem_init(&queue_info->sem_write, 1, 1);
        sem_init(&queue_info->sem_read, 1, 1);

    }

    QueueData *queue = mmap(NULL, queue_info->chunk_size * sizeof(QueueData), PROT_READ | PROT_WRITE,
                            MAP_SHARED, fd_queue, 0);

    close(fd_queue);
    close(fd_info);
    close(fd_stats);
    
    stats->encoders_counter += 1;

    write_info(queue, queue_info, stats, images, imageName, chunkSize, mode, key, stats->total_enco);

    stats->encoders_counter -= 1;
    unsigned long currRealMem, peakRealMem, currVirtMem, peakVirtMem;
    int success = getMemory(&currRealMem, &peakRealMem, &currVirtMem, &peakVirtMem, stats);

    if (stats->encoders_counter == 0 && stats->decoders_counter == 0)
    {
        int callStats = system("./stats");
    }
    
    return 0;
}