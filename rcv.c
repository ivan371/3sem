#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#define BUF_SIZE 10

ssize_t getpipesize(void);

int main (int argc, char** argv)
{
    if (argc != 1) {
        printf ("ERROR IN ARGC\n");
        exit(-1);
    }

    mkfifo ("fifo_rcv", 0644);
    mkfifo ("fifo_snd", 0644);

    int fifo_snd_d = open ("fifo_snd", O_NONBLOCK, O_RDONLY);
    int fifo_rcv_d = open ("fifo_rcv", O_WRONLY);
    ssize_t fifo_rcv_size = getpipesize();
    void* buf_rcv = (void*)calloc (fifo_rcv_size, 1);
    fcntl (fifo_rcv_d, F_SETFL, O_NONBLOCK);
    if (write (fifo_rcv_d, buf_rcv, fifo_rcv_size) == -1) {
        printf ("BAD RECIEVER: %d\n", getpid());
        return 0;
    }


    mkfifo ("fifo", 0644);
    int fifo_d = open ("fifo", O_RDONLY);
    void* buf = calloc (BUF_SIZE, 1);

    int nBytes = BUF_SIZE;
    while (nBytes != 0) {
        nBytes = read (fifo_d, buf, BUF_SIZE);
        if(write (STDOUT_FILENO, buf, nBytes) < 0)
	{
		printf("WRITE ERROR");
		exit(-1);
	}
    }

    unlink ("fifo_rcv");
    unlink ("fifo_snd");
    unlink ("fifo");
    free  (buf);
    close (fifo_snd_d);
    close (fifo_rcv_d);
    close (fifo_d);

    return 0;
}

ssize_t getpipesize(void)
{
        ssize_t nbytes, pipesize;
        int pfds[2];
        char buf[PIPE_BUF * 2];
 
        if ((pipe(pfds)) == -1)
                return (-1);
 
#ifdef F_GETPIPE_SZ
        if ((nbytes = fcntl(pfds[1], F_GETPIPE_SZ, 0)) == -1) {
#endif
 
        if (fcntl(pfds[1], F_SETFL, O_NONBLOCK) == -1)
                pipesize = -1;
        else {
                pipesize = 0;
                while ((nbytes = write(pfds[1], &buf, sizeof buf)) > 0)
                        pipesize += nbytes;
        }
 
#ifdef F_GETPIPE_SZ
        } else
                pipesize = nbytes;
#endif
 
        close(pfds[0]);
        close(pfds[1]);
 
        return (pipesize);
}

