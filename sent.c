#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024
#define FIFO_DIR "FIFO"

#define ERROR(x) \
    do { printf(x "\n");  exit(-1);} while(NULL)


int main(int argc, char** argv)
{
    int fifo_fd = 0;
    int file_fd = 0;
    int length = 0;
    int tmp = 0;
    int sync_pid = 0;
    int err = 0;

    char buffer[BUFFER_SIZE] = {0};
    char uniq_fifo[BUFFER_SIZE] = {};

    file_fd = open(argv[1], O_NONBLOCK);
    if(file_fd < 0)
	ERROR("Open file [argv1] error");
    
    if(mkfifo(FIFO_DIR, 0666) < 0)
	//ERROR("Mkfifo sync fifo error");

    fifo_fd = open(FIFO_DIR, O_RDONLY);
    if(fifo_fd == -1)
	ERROR("Open sync fifo error");

    if(read(fifo_fd, uniq_fifo, sizeof(char*)) < 0)
	ERROR("Read sync file error");

    close(fifo_fd);
    if(unlink(FIFO_DIR) == -1)
	//ERROR("Delete fifo error");

    fifo_fd = open(uniq_fifo, O_WRONLY);
    if(fifo_fd < 0)
	ERROR("Open transfer fifo error");

    while((length = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
	if(write(fifo_fd, buffer, length) < 0)
	    ERROR("Write error");
    }

    close(fifo_fd);
    close(file_fd);

    return 0;
}
