#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
//

//Globals
void *shmem = NULL;

int
main()
{
    // open the file descriptor, show an error if it does not exist
    int fd = open("/tmp/project_1",O_RDWR,0644);
    if (fd < 0) {
        perror("project_1");
        exit(1);
    }

    // amount of elements in the array
    int N = 10;

    // memory mapping function instance
    //      address: NULL means the kernel can place the mapping anywhere it sees fit
    //      lenght: number of bytes to be mapped, the array will contain 10 integers
    //      protect: PROT_READ | PROT_WRITE the access allows reading and writing on the content
    //      flags: MAP_SHARED this flag is used to share the mapping with all other processes,
    //          which are mapped to this object
    //      filedes: file descriptor to be mapped
    //      offset: 0, offset from where the mapping started

    // source: https://linuxhint.com/using_mmap_function_linux/
    int *ptr = mmap(NULL, N*sizeof(int), PROT_READ | PROT_WRITE,
        MAP_SHARED, fd, 0);

    // print the values in memory
    printf("Updated values of the array elements :\n");
    for (int i = 0; i < N; i++ ){
        printf(" %d", ptr[i] );
    }
    printf("\n");

    // close the file descriptor
    close(fd);

    // stop the shared memory
    unlink("/tmp/project_1");

    return 0;
}