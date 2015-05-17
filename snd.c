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
    if (argc != 2) {
        printf ("ERROR IN ARGC\n");
        exit(-1);
    }

    mkfifo ("fifo_snd", 0644);
    mkfifo ("fifo_rcv", 0644);

    int fifo_rcv_d = open ("fifo_rcv", O_NONBLOCK, O_RDONLY);
    int fifo_snd_d = open ("fifo_snd", O_WRONLY);
    size_t fifo_snd_size = getpipesize();
    void* buf_snd = (void*)calloc (fifo_snd_size, 1);
    fcntl (fifo_snd_d, F_SETFL, O_NONBLOCK);
    if (write (fifo_snd_d, buf_snd, fifo_snd_size) == -1) {
        printf ("BAD SENDER %d\n", getpid());
        exit(1);
    }

    mkfifo ("fifo", 0644);

    int fifo_d = open ("fifo", O_WRONLY);
    int input_d = open (argv[1], O_RDONLY);
    void* buf = calloc (BUF_SIZE, 1);
    int nBytes = BUF_SIZE;
    while (nBytes == BUF_SIZE) {
        nBytes = read (input_d, buf, BUF_SIZE);
        if(write (fifo_d, buf, nBytes) < 0)
	{
		printf("WRITE ERROR");
		exit(-1);
	}
    }

    close (input_d);
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

