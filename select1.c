#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>

#define ERROR(x) do{printf(x); exit(1);} while(0);

struct pipes {
   	int rfd;
    	int wfd;
    	void* buf;
    	int bufsz;
    	int used_space;
    	int can_read;
    	int can_write;
} pipes;

void parent (struct pipes* pipes, int* pipefds, int nForks);
void parent_prepare_fd_sets (struct pipes *pipes, fd_set *rfds, fd_set *wfds, int *fd_max, int nForks);
void parent_rdwr (struct pipes *pipes, fd_set *rfds, fd_set *wfds, int nFds, int nForks);


int pipes_read  (struct pipes *pipe);
int pipes_write (struct pipes *pipe);

void child (int* piper, int* pipew);
void child_close_fds (int* pipefds, int* rpipe, int num);

void dump_all (struct pipes* pipe, int nForks);
void dump (struct pipes* pipe);

int main(int argc, char* argv[])
{
	if(argc != 3)
		ERROR("NOT_3_ARGC");
	char* end_ptr = NULL;
	int nForks = strtol (argv[2], &end_ptr, 10);
	

	int i = 0;
   	int *pipefds = (int*) calloc (4 * nForks - 3, sizeof (int));
	int piper[2];
	int pipew[2];
	int rpipe[2];
	int wpipe[2];



	int input_d = open (argv[1], O_RDONLY);
    	if (input_d == -1)
		 ERROR("INPUT_D ERROR");

	pid_t pid;
	pipe(rpipe);
	pipefds[0] = rpipe[0];
	pipefds[1] = rpipe[1];
	pid = fork();
    	if (!pid) {
		if (close (rpipe[0]) == -1) ERROR("CLOSE ERROR");
		void* buf = calloc (1024, sizeof(void));
    		int nBytes = -1;
    		while (nBytes) {
        		nBytes = read (input_d, buf, 1024);
        		if (write (rpipe[1], buf, nBytes) == -1)
			{ 
				ERROR("WRITE ERROR")
			}    
		}
    		free (buf);
		if (close (rpipe[1]) == -1) ERROR("CLOSE ERROR");
		if (close (input_d) == -1) ERROR("CLOSE ERROR")
		exit(1);  	
	}
	int j = 0;
    	for (i = 1; i < nForks; i++) {
		pipe(piper);
		pipe(pipew);
		pipefds[4 * i - 2] = pipew[0];
		pipefds[4 * i - 1] = pipew[1];
		pipefds[4 * i] = piper[0];
		pipefds[4 * i + 1] = piper[1];
        	pid = fork();
        	if (!pid) 
		{
			child_close_fds (pipefds, rpipe, i);			
			child (piper, pipew);
		}
	}
    



	struct pipes* pipes = (struct pipes*)calloc (nForks, sizeof (struct pipes));
	parent (pipes, pipefds, nForks);

	for (i = 0; i < nForks; i++) {
        	free (pipes[i].buf);
        	pipes[i].bufsz = -1;
    	}

   	free (pipes);
   	free (pipefds);
   	return 0;
}

void parent (struct pipes *pipes, int *pipefds, int nForks)
{
    	int i = 0;
   	for (i = 0; i < 2 * nForks - 1; i++) {
		if (i % 2 == 0) pipes[i / 2].rfd = pipefds[2 * i];
        	else {
            		pipes[(i - 1) / 2].wfd = pipefds[2 * i + 1];
           		if (fcntl (pipefds[2 * i + 1], F_SETFL, O_NONBLOCK) == -1) ERROR("FCNTL ERROR1");
			if (fcntl (pipefds[2 * i + 2], F_SETFL, O_NONBLOCK) == -1) ERROR("FCNTL ERROR2");
        	}
    	}
	if (fcntl (pipefds[0], F_SETFL, O_NONBLOCK) == -1) ERROR("FCNTL ERROR1");
    	pipes[nForks - 1].wfd = STDOUT_FILENO;

    	for (i = 0; i < nForks; i++) {
        	pipes[i].bufsz = 1111 * (i + 1);
        	pipes[i].buf = calloc (pipes[i].bufsz, 1);
        	pipes[i].used_space = 0;
        	pipes[i].can_read = 1;
    	}

    	for (i = 0; i < 2 * nForks; i++) {
        	if (i % 2 == 0) close (pipefds[2 * i + 1]);
        	else            close (pipefds[2 * i]);
    	}

    	fd_set rfds_obj, wfds_obj;
    	fd_set *rfds = &rfds_obj, *wfds = &wfds_obj;
    	int fd_max = -1, nFds = 0;

    	while (1) {
        	parent_prepare_fd_sets (pipes, rfds, wfds, &fd_max, nForks);
       		if (fd_max == -1) break;		
		//dump_all (pipes, nForks);
        	nFds = select (fd_max + 1, rfds, wfds, NULL, NULL);
        	if(nFds == -1)
		 	ERROR("SELECT ERROR");
        	parent_rdwr (pipes, rfds, wfds, nFds, nForks);
		//dump_all (pipes, nForks);
    }
}

void parent_prepare_fd_sets (struct pipes *pipes, fd_set *rfds, fd_set *wfds, int *fd_max, int nForks)
{
    	int i = 0;
    	FD_ZERO (rfds);
    	FD_ZERO (wfds);
    	*fd_max = -1;

    	for (i = 0; i < nForks; i++) {
        	if (pipes[i].can_read == 1) {
        		FD_SET (pipes[i].rfd, rfds);
        	    	if (pipes[i].rfd > *fd_max) *fd_max = pipes[i].rfd;
        	}
        	if (pipes[i].can_write == 1) {
        	    	FD_SET (pipes[i].wfd, wfds);
        	    	if (pipes[i].wfd > *fd_max) *fd_max = pipes[i].wfd;
        	}
    	}

}

void parent_rdwr (struct pipes *pipes, fd_set *rfds, fd_set *wfds, int nFds, int nForks)
{
    	int i = 0;
    	int nChecked_fds = 0;
    	i = 0;
    	while (nChecked_fds < nFds) {
       		if (FD_ISSET (pipes[i].rfd, rfds)) {
            		if (!pipe_read  (&pipes[i])) {
            	    		if(close (pipes[i].rfd) == -1) ERROR("CLOSE ERROR5");
    	        	}
    	        	nChecked_fds++;
    	   	}
    	   	if (FD_ISSET (pipes[i].wfd, wfds)) {
    	        	pipe_write (&pipes[i]);
    	        	if (pipes[i].can_write == 2 && pipes[i].wfd != STDOUT_FILENO)
			{
			 	if (close (pipes[i].wfd) == -1) ERROR("CLOSE ERROR6");
			}
    	       		nChecked_fds++;
    	    	}
    	    	i++;
    	}
}

int pipe_read  (struct pipes *pipe)
{
    	int   rfd        = pipe -> rfd;
    	void* buf        = pipe -> buf;
    	int   used_space = pipe -> used_space;
    	int   bufsz      = pipe -> bufsz;

    	int res = read (rfd, &buf[used_space], bufsz - used_space);
    	if (res == -1) ERROR("READ FAILED");

    	pipe -> used_space += res;
    	if (res + used_space == bufsz) pipe -> can_read = 0;
    	else if (!res) pipe -> can_read = 2;
    	pipe -> can_write = 1;
	//dump(pipe);

    	return res;
}

int pipe_write  (struct pipes *pipe)
{
    	int   wfd        = pipe -> wfd;
    	void* buf        = pipe -> buf;
    	int   used_place = pipe -> used_space;

   	int res = 0;
    	int nBytes = used_place;
	res = write (wfd, buf, nBytes);
	if (res != used_place) {
        	int i = 0;
        	for (i = res; i < used_place; i++) ((char*)buf)[i - res] = ((char*)buf)[i];
        	pipe -> can_read = 1;
    	}
    	else 
	{
		if (pipe -> can_read == 2) pipe -> can_write = 2;
	        else {
             		pipe -> can_write = 0;
             		pipe -> can_read  = 1;
        	}
	}

    	pipe -> used_space -= res;
    	return res;
}

void dump_all (struct pipes* pipe, int nForks)
{
	int i = 0;
	for(i = 0; i < nForks; i++)
	{
    		printf ("rfd\t   = %d\n wfd\t   = %d\n\n", pipe[i].rfd, pipe[i].wfd);
    		printf ("buf\t   = [%p]\nbufsz\t   = %d\nused_space = %d\n", pipe[i].buf, pipe[i].bufsz, pipe[i].used_space);
    		printf ("can_read   = %d\ncan_write  = %d\n",  pipe[i].can_read, pipe[i].can_write);
	}
}

void dump (struct pipes* pipe)
{
	printf ("rfd\t   = %d\n wfd\t   = %d\n\n", pipe -> rfd, pipe -> wfd);
    	printf ("buf\t   = [%p]\nbufsz\t   = %d\nused_space = %d\n", pipe -> buf, pipe -> bufsz, pipe -> used_space);
 	printf ("can_read   = %d\ncan_write  = %d\n",  pipe -> can_read, pipe -> can_write);
}

void child_close_fds (int* pipefds, int* rpipe, int num)
{
	if (close (rpipe[0]) == -1) ERROR("CLOSE ERROR");
	if (close (rpipe[1]) == -1) ERROR("CLOSE ERROR");
	int j = 1;
	for (j = 1; j < num; j++)
	{
		if (close (pipefds[4 * j - 2]) == -1) ERROR("CLOSE ERROR");
		if (close (pipefds[4 * j - 1]) == -1) ERROR("CLOSE ERROR");
		if (close (pipefds[4 * j]) == -1) ERROR("CLOSE ERROR");
		if (close (pipefds[4 * j + 1]) == -1) ERROR("CLOSE ERROR");
	}
}

void child (int* piper, int* pipew)
{
	int errno;
	if (close (piper[0]) == -1) ERROR("CLOSE ERROR");
	if (close (pipew[1]) == -1) ERROR("CLOSE ERROR"); 
    	void* buf = calloc (1024, sizeof(void));
    	int nBytes = -1;
    	while (nBytes) {
        	nBytes = read (pipew[0], buf, 1024);
        	if (write (piper[1], buf, nBytes) == -1)
		{ 
			ERROR("WRITE ERROR")
		}    
	}
    	free (buf);
	if (close (piper[1]) == -1) ERROR("CLOSE ERROR");
	if (close (pipew[0]) == -1) ERROR("CLOSE ERROR");
	exit(1);
}
