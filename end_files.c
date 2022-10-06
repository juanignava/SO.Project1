#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
int main(int argc, char const *argv[])
{
    unlink("/tmp/project_1_queue");
    unlink("/tmp/project_1_info");
    unlink("/tmp/project_1_stats");
    return 0;
}
