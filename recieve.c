#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define BUFFER_SIZE 1024
#define FIFO_DIR "FIFO"
#define STR_LEN 256

#define ERROR(x) \
    do { printf(x "\n"); exit(-1); } while(NULL)

int main(int argc, char** argv)
{
    int fifo_fd = 0;
    int file_fd = 0;
    int length = 0;

    char buffer[BUFFER_SIZE] = {0};
    char uniq_name[STR_LEN] = FIFO_DIR;
	
    pid_t pid = getpid();
    snprintf(uniq_name, sizeof(FIFO_DIR), "%d", pid);

    if(mkfifo(uniq_name, 0666) < 0)
	ERROR("Mkfifo error");

    fifo_fd = open(FIFO_DIR, O_WRONLY);
    if(fifo_fd < 0)
	ERROR("Open sync fifo error");

    if(write(fifo_fd, uniq_name, sizeof(char*)) < 0)
	ERROR("Write to sync fifo error");

    close(fifo_fd);

    fifo_fd = open(uniq_name, O_RDONLY);
    if(fifo_fd < 0)
	ERROR("Open transfer fifo error");

    while((length = read(fifo_fd, buffer, BUFFER_SIZE)) > 0) {
	if(write(STDOUT_FILENO, buffer, length) < 0)
	    ERROR("Read error");
    }

    if(length < 0)
	ERROR("Read error");

    close(uniq_name);
    unlink(uniq_name);
    return 0;
}
