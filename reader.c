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
//int chunk = 10;

// STRUCT
typedef struct
{
    unsigned char value;
    int id;
    time_t raw_pixel_time;
} QueueData;


typedef struct
{
    sem_t sem_filled;
    sem_t sem_empty;
    int next_input;
    int next_output;
    int chunk_size;
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
    int total_enco;
    int total_deco;
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

void buildImage(int width, int height, int channels, unsigned char *pixels)
{
    printf("Creating the image\n");
    stbi_write_jpg("image2.jpg", width, height, channels, pixels, width*channels);
}

void read_info_auto(QueueData *queue, QueueInfo *queue_info, Stats *stats, char* mode, int* key, int id)
{
    stats->total_deco++;
    // Get image dimensions and data for the analysis
    int width, height, channels;
    unsigned char *img = stbi_load("image.jpg", &width, &height, &channels, 0);
    unsigned char img2[width * height*channels];
    int clave = key;

    for (int i = 0; i < width * height * channels; i++)
    {

        if (queue[queue_info->next_output].id == id)
        {
            clock_t begin_sem = clock();
            sem_wait(&queue_info->sem_empty);
            clock_t end_sem = clock();

            clock_t begin = clock();
            unsigned char val = queue[queue_info->next_output].value;
            printf("Reading value: %d in position: %d\n", val, i);
            int timeInfo = getTime(queue->raw_pixel_time);
            val = val ^ getDecimal(clave);
            img2[i] = val;
            queue_info->next_output = (i+1) % queue_info->chunk_size; // for circular list
            clock_t end = clock();

            // Adding reading time
            stats->total_kernel_time += (double)(end - begin) / CLOCKS_PER_SEC; // Adding kernel time to stats
            stats->blocked_sem_time += (double)(end_sem - begin_sem) / CLOCKS_PER_SEC; // Adding blocked sem time to stats

            sem_post(&queue_info->sem_filled);
        }
        else
        {
            sem_post(&queue_info->sem_empty);
        }


        

        // wait for an enter hit when mode is manual
        if (strcmp(mode, "manual") == 0)
        {
            char received_char = getchar();
            if (received_char == 'q')
            {
                mode = "auto";
            }
            
        }
        
    }

    buildImage(width, height, channels, img2);
    
}

/*
 * Measures the current (and peak) resident and virtual memory
 * usage of your linux C process, in bytes, accurate to nearest kB.
 * Returns a 0 if memory info access was successful, else prints
 * an error message and returns 1
 */
int getMemory( unsigned long *currRealMem, unsigned long *peakRealMem, unsigned long *currVirtMem, unsigned long *peakVirtMem, Stats *stats) {
 
    // stores each word in status file
    char buffer[1024] = "";
    
    // linux file contains this-process info
    FILE* file = NULL;
    file = fopen("/proc/self/status", "r");

    if (file == NULL) {
        printf("Call to getMemory FAILED; "
            "linux file proc/self/status not found!\n");
        return 1;
    }

    // read the entire file, recording mems in kB
    while (fscanf(file, " %1023s", buffer) == 1) {
    
        if (strcmp(buffer, "VmRSS:") == 0) {
            fscanf(file, " %lu", currRealMem);
        }
        if (strcmp(buffer, "VmHWM:") == 0) {
            fscanf(file, " %lu", peakRealMem);
        }
        if (strcmp(buffer, "VmSize:") == 0) {
            fscanf(file, " %lu", currVirtMem);
        }
        if (strcmp(buffer, "VmPeak:") == 0) {
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

int getTime(time_t rawtime){
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    //printf ( "Current raw time 2: %ld\n", rawtime);
    printf ( "Pixel saved local time and date: %s\n", asctime (timeinfo) );

}

int main(int argc, char *argv[])
{
    // Se declaran variables de argumentos
    char mode[50];
    strcpy(mode, "");
    int key = -1;
    // Se leen los argumentos
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-m") == 0) {
            strcpy(mode, argv[i + 1]);
        }
        if (strcmp(argv[i], "-k") == 0) {
            key = atoi(argv[i + 1]);
        }   
    }

    if (strcmp(mode, "") == 0 || key < 1) {
        printf("No se pudo determinar el modo o la clave\n");
        return 1;
    }
    
    
    // opens the file descriptor that has to be mapped to the
    //     shared memory
    
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

    
    QueueInfo *queue_info = mmap(NULL, sizeof(QueueInfo), PROT_READ | PROT_WRITE,
        MAP_SHARED, fd_info, 0);

    Stats *stats = mmap(NULL, sizeof(Stats), PROT_READ | PROT_WRITE,
        MAP_SHARED, fd_stats, 0);

    int fd_queue = open("/tmp/project_1_queue", O_RDWR | O_CREAT, 0644);
    if (fd_queue < 0) {
        perror("project_1_queue");
        exit(1);
    }
    ftruncate(fd_queue, queue_info->chunk_size *sizeof(QueueData));

    QueueData *queue =  mmap(NULL, queue_info->chunk_size *sizeof(QueueData), PROT_READ | PROT_WRITE,
        MAP_SHARED, fd_queue, 0);

    stats->decoders_counter += 1;
    
    // fill the queue with the data

    read_info_auto(queue, queue_info, stats, mode, key, stats->total_deco);

    unlink("/tmp/project_1_queue");
    unlink("/tmp/project_1_info");

    stats->decoders_counter -= 1;
    unsigned long currRealMem, peakRealMem, currVirtMem, peakVirtMem;
    int success = getMemory(&currRealMem, &peakRealMem, &currVirtMem, &peakVirtMem, stats);

    if (stats->encoders_counter == 0 && stats->decoders_counter == 0){
        int callStats = system("./stats");
    }

    unlink("/tmp/project_1_stats");
   
    close(fd_queue);
    close(fd_info);
    close(fd_stats);
    return 0;
}