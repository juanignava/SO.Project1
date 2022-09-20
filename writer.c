#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>

// CONSTANTS

// number of elements in the array
int N = 10;

int main()
{
    // opens the file descriptor that has to be mapped to the
    //     shared memory
    int fd = open("/tmp/project_1", O_RDWR | O_CREAT, 0644);
    ftruncate(fd, N*sizeof(int));

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

    // print initial values of the shared memory
    printf("Initial values of the array elements :\n");
    for (int i = 0; i < N; i++ ){
        printf(" %d", ptr[i] );
    }
    printf("\n");

    // change the values on the shared memory
    for(int i=0; i < N; i++){
        ptr[i] = i + 1;
    }

    // close the file descriptor
    close(fd);

    return 0;
}