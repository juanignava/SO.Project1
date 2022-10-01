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

int main()
{
    int fd_stats = open("/tmp/project_1_stats", O_RDWR | O_CREAT, 0644);
    if (fd_stats < 0) {
        perror("project_1_stats");
        exit(1);
    }
    ftruncate(fd_stats, sizeof(Stats));

    Stats *stats = mmap(NULL, sizeof(Stats), PROT_READ | PROT_WRITE,
        MAP_SHARED, fd_stats, 0);

    // Printing stats
    // Printing Memory usage values
    printf("\033[1;33m");
    printf("\n[+] Memory: \n");

    printf("\033[1;35m");
    printf("    ->");
    printf("\033[0m");
    printf(" Total real memory usage: %li bytes\n", stats->real_memory_used);

    printf("\033[1;35m");
    printf("    ->");
    printf("\033[0m");
    printf(" Total virtual memory usage: %li bytes\n", stats->virtual_memory_used);

    // Printing Time values
    printf("\033[1;32m");
    printf("\n[+] Time: \n");

    printf("\033[1;35m");
    printf("    ->");
    printf("\033[0m");
    printf(" Total sem blocked time: %f miliseconds\n", stats->blocked_sem_time*1000);

    printf("\033[1;35m");
    printf("    ->");
    printf("\033[0m");
    printf(" Total kernel time: %f miliseconds\n", stats->total_kernel_time*1000);

    // Printing Data values
    printf("\033[1;34m");
    printf("\n[+] Data: \n");

    printf("\033[1;35m");
    printf("    ->");
    printf("\033[0m");
    printf(" Total pixels processed: %i\n", stats->total_pixels_processed);

    printf("\033[1;35m");
    printf("    ->");
    printf("\033[0m");
    printf(" Total pixels greater than 175: %i\n", stats->pixels_greater_than_175);

    close(fd_stats);
    return 0;
}