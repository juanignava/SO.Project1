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

#define NUM_ITEMS 180

// GLOBALS

int chunk = 10;

// STRUCT
typedef struct
{
    unsigned char value;
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
    unsigned long real_memory_used;
    unsigned long virtual_memory_used;
    double blocked_sem_time;
    int total_pixels_processed;
    double total_kernel_time;
    int pixels_greater_than_175;
    int encoders_counter;
    int decoders_counter;
} Stats;

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
void write_info_auto(QueueData *queue, QueueInfo *queue_info, Stats *stats)
{
    int width, height, channels;
    printf("Before reading image\n");
    unsigned char *img = stbi_load("image.jpg", &width, &height, &channels, 0);
    printf("After reading image\n");
    int clave = 10101001;

    for (int i = 0, j = 0; i < width * height * channels; i++, j++)
    {
        clock_t begin_sem = clock();
        sem_wait(&queue_info->sem_filled);
        clock_t end_sem = clock();

        printf("Adding value: %d in position: %d\n", img[i] ^ getDecimal(clave), i);

        clock_t begin = clock();
        unsigned char encoded = img[i] ^ getDecimal(clave);
        queue[queue_info->next_input].value = encoded;
        //queue[queue_info->next_input].value = i;
        queue_info->next_input = (j + 1) % chunk; // for circular list
        stats->total_pixels_processed += 1;       // Adding 1 to total pixels encoded (for stats)
        clock_t end = clock();

        stats->total_kernel_time += (double)(end - begin) / CLOCKS_PER_SEC;        // Adding kernel time (for stats)
        stats->blocked_sem_time += (double)(end_sem - begin_sem) / CLOCKS_PER_SEC; // Adding blocked sem time to stats

        if ((int)img[i] >= 176)
        {
            stats->pixels_greater_than_175 += 1; // Adding 1 to pixels greater than 175 (for stats)
        }

        sem_post(&queue_info->sem_empty);
    }
}

void write_info_manual(QueueData *queue, QueueInfo *queue_info, Stats *stats)
{
    int width, height, channels;
    printf("Before reading image\n");
    unsigned char *img = stbi_load("image.pgm", &width, &height, &channels, 0);
    printf("After reading image\n");
    int clave = 10101001;
    for (int i = 0, j = 0; i < NUM_ITEMS * (channels-1); i=i+channels-1, j++)
    {
        clock_t begin_sem = clock();
        sem_wait(&queue_info->sem_filled);
        clock_t end_sem = clock();

        printf("Adding value: %d\n", i);

        clock_t begin = clock();
        int encoded = (int)img[i] ^ getDecimal(clave);
        queue[queue_info->next_input].value = encoded;
        //queue[queue_info->next_input].value = i;
        queue_info->next_input = (j + 1) % chunk; // for circular list
        stats->total_pixels_processed += 1;       // Adding 1 to total pixels encoded (for stats)
        clock_t end = clock();

        stats->total_kernel_time += (double)(end - begin) / CLOCKS_PER_SEC;        // Adding kernel time (for stats)
        stats->blocked_sem_time += (double)(end_sem - begin_sem) / CLOCKS_PER_SEC; // Adding blocked sem time to stats

        if ((int)img[i] >= 176)
        {
            stats->pixels_greater_than_175 += 1; // Adding 1 to pixels greater than 175 (for stats)
        }

        sem_post(&queue_info->sem_empty);
        getchar();
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

int main(int argc, char *argv[])
{
    // opens the file descriptor that has to be mapped to the
    //     shared memory
    int fd_queue = open("/tmp/project_1_queue", O_RDWR | O_CREAT, 0644);
    if (fd_queue < 0)
    {
        perror("project_1_queue");
        exit(1);
    }
    ftruncate(fd_queue, chunk * sizeof(QueueData));

    int fd_info = open("/tmp/project_1_info", O_RDWR | O_CREAT, 0644);
    if (fd_info < 0)
    {
        perror("project_1_info");
        exit(1);
    }
    ftruncate(fd_info, sizeof(QueueInfo));

    int fd_stats = open("/tmp/project_1_stats", O_RDWR | O_CREAT, 0644);
    if (fd_stats < 0)
    {
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
    QueueData *queue = mmap(NULL, chunk * sizeof(QueueData), PROT_READ | PROT_WRITE,
                            MAP_SHARED, fd_queue, 0);

    QueueInfo *queue_info = mmap(NULL, sizeof(QueueInfo), PROT_READ | PROT_WRITE,
                                 MAP_SHARED, fd_info, 0);

    Stats *stats = mmap(NULL, sizeof(Stats), PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd_stats, 0);

    // sem_filled begins in chunk because everuthing is empty
    sem_t sem_filled;
    sem_init(&sem_filled, 1, chunk);
    // sem_empty begins in 0 because everuthing is empty
    sem_t sem_empty;
    sem_init(&sem_empty, 1, 0);
    int next_input = 0;
    int next_output = 0;

    queue_info->sem_filled = sem_filled;
    queue_info->sem_empty = sem_empty;
    queue_info->next_input = 0;
    queue_info->next_output = 0;

    stats->encoders_counter += 1;

    if (strcmp(argv[1], "auto") == 0)
    {
        write_info_auto(queue, queue_info, stats);
    }

    else if (strcmp(argv[1], "manual") == 0)
    {
        write_info_manual(queue, queue_info, stats);
    }
    else
    {
        printf("Indicate a valid operation method: manual or auto");
    }

    stats->encoders_counter -= 1;
    unsigned long currRealMem, peakRealMem, currVirtMem, peakVirtMem;
    int success = getMemory(&currRealMem, &peakRealMem, &currVirtMem, &peakVirtMem, stats);

    if (stats->encoders_counter == 0 && stats->decoders_counter == 0)
    {
        int callStats = system("./stats");
    }
    close(fd_queue);
    close(fd_info);
    close(fd_stats);
    return 0;
}