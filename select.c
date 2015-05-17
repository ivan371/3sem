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

void parent (struct pipes* pipes, int* pipefds, int nForks, int *piper, int* pipew, int* num_pipe, int* write_pipe, int* read_pipe);
void parent_prepare_fd_sets (struct pipes *pipes, fd_set *rfds, fd_set *wfds, int *fd_max, int nForks);
void parent_rdwr (struct pipes *pipes, fd_set *rfds, fd_set *wfds, int nFds, int nForks);


int pipes_read  (struct pipes *pipe);
int pipes_write (struct pipes *pipe);

int child_close_extr_fds (int* pipefds, int nChild, int nForks);
void child (int fd_1, int fd_2);
void child_and_operations (int fd_1, int fd_2, int* pipefds, int nForks, int num, int* piper, int* pipew);

void dump_all (struct pipes* pipe, int nForks);
void dump (struct pipes* pipe);

int main(int argc, char* argv[])
{
	if(argc != 3)
		ERROR("NOT_3_ARGC");
	char* end_ptr = NULL;
	int nForks = strtol (argv[2], &end_ptr, 10);
	

	int i = 0;
   	int *pipefds = (int*) calloc (4 * nForks - 1, sizeof (int));
	int piper[2];
	int pipew[2];
	int num_pipe[2];
	int read_pipe[2];
	int write_pipe[2];
	pipe(num_pipe);
	pipe(read_pipe);
	pipe(write_pipe);
	
   	/*for (i = 0; i < 2 * nForks; i++) 
	{
		if (pipe (&(pipefds[2 * i])) != 0) 
			ERROR("PIPE ERROR1");
	}*/



	int input_d = open (argv[1], O_RDONLY);
    	if (input_d == -1)
		 ERROR("INPUT_D ERROR");

	pid_t pid, newpid;
	pid = fork();
    	if (!pid) {
		pipe(piper);
		pipe(pipew);
		newpid = fork();
		if(newpid == 0)
			child_and_operations(input_d, piper[1], pipefds, nForks, 0, piper, pipew);    
			
	}
	else
	{
    		for (i = 1; i < nForks; i++) {
        		pid = fork();
			printf("HERE");
        		if (!pid)
			{ 
				pipe(piper);
				pipe(pipew);
				close(num_pipe[0]);
				close(write_pipe[0]);
				close(read_pipe[0]);
				write(num_pipe, sizeof(i), i);
				write(write_pipe, sizeof(piper[0]), piper[0]);
				write(read_pipe, sizeof(pipew[0]), pipew[0]);
				child_and_operations(pipew[0], piper[1], pipefds, nForks, i, piper, pipew);
				exit(1);
			}			
			printf("MAIN");
		}
	}
    

	struct pipes* pipes = (struct pipes*)calloc (nForks, sizeof (struct pipes));
	parent (pipes, pipefds, nForks, piper, pipew, num_pipe, write_pipe, read_pipe);

	for (i = 0; i < nForks; i++) {
        	free (pipes[i].buf);
        	pipes[i].bufsz = -1;
    	}

   	free (pipes);
   	//free (pipefds);
   	return 0;
}

void parent (struct pipes *pipes, int *pipefds, int nForks, int *piper, int *pipew, int* num_pipe, int* write_pipe, int* read_pipe)
{
    	int i = 0;
	int nChild = 0;
	int writer = 0;
	int reader = 0;
	while(i != nForks)
	{
		close(num_pipe[1]);
		close(write_pipe[1]);
		close(read_pipe[1]);
		read(num_pipe, nForks, nChild);
		read(write_pipe, PIPE_BUF, writer);
		read(read_pipe, PIPE_BUF, reader);
		pipes[nChild].rfd = reader;
		if(nChild != nForks)	
			pipes[nChild].wfd = writer;
		else
			pipes[nForks].wfd = STDOUT_FILENO;

		if (fcntl (piper[0], F_SETFL, O_NONBLOCK) == -1) ERROR("FCNTL ERROR1");
		if (fcntl (piper[1], F_SETFL, O_NONBLOCK) == -1) ERROR("FCNTL ERROR2");
		if (fcntl (pipew[0], F_SETFL, O_NONBLOCK) == -1) ERROR("FCNTL ERROR3");
		if (fcntl (pipew[1], F_SETFL, O_NONBLOCK) == -1) ERROR("FCNTL ERROR4");
		i++;
	}
   	/*for (i = 0; i < 2 * nForks; i++) {
		if (i % 2 == 0) pipes[i / 2].rfd = pipefds[2 * i];
        	else {
            		pipes[(i - 1) / 2].wfd = pipefds[2 * i + 1];
           		if (fcntl (pipefds[2 * i + 1], F_SETFL, O_NONBLOCK) == -1) ERROR("FCNTL ERROR1");
			if (fcntl (pipefds[2 * i + 2], F_SETFL, O_NONBLOCK) == -1) ERROR("FCNTL ERROR2");
        	}
    	}*/

    	//if(close (pipefds [4 * nForks - 1]) == -1) ERROR("CLOSE ERROR3");
    	//if(close (pipefds [4 * nForks - 2]) == -1) ERROR("CLOSE ERROR4");
	//close(pipew[1]);
	//close(pipew[0]);
    	//pipes[nForks - 1].wfd = STDOUT_FILENO;

    	for (i = 0; i < nForks; i++) {
        	pipes[i].bufsz = 1111 * (i + 1);
        	pipes[i].buf = calloc (pipes[i].bufsz, 1);
        	pipes[i].used_space = 0;
        	pipes[i].can_read = 1;
    	}

    	//for (i = 0; i < 2 * nForks - 1; i++) {
        //	if (i % 2 == 0) close (pipefds[2 * i + 1]);
        //	else            close (pipefds[2 * i]);
    	//}
	if(close(piper[1]) == -1) ERROR("CLOSE_ERROR1");
	if(close(pipew[0]) == -1) ERROR("CLOSE_ERROR2");

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
    	while (1) {
        	res = write (wfd, buf, nBytes);
        	if (res == -1) {
            		nBytes /= 2;
        	}
        	else 
			break;
    	}

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

void child_and_operations (int fd_1, int fd_2, int* pipefds, int nForks, int num, int* piper, int* pipew)
{
	printf("IN_CHILD");
	//child_close_extr_fds (pipefds, num, nForks);
	if (close (piper[0]) == -1)  ERROR( "CLOSE ERROR1");
	if(nForks != 0)
	{        	
		if (close (pipew[1]) == -1)  ERROR( "CLOSE ERROR2");
	}	
	else
	{	
		if (close (pipew[0]) == -1)  ERROR( "CLOSE ERROR2");
		if (close (pipew[1]) == -1)  ERROR( "CLOSE ERROR2");
	}
        child (fd_1, fd_2);
   	if (close (fd_1) == -1)  ERROR( "CLOSE ERROR1");
        if (close (fd_2) == -1)  ERROR( "CLOSE ERROR2");
}

int child_close_extr_fds (int* pipefds, int nChild, int nForks)
{
 	int i = 0;

    	if (nChild != 0) {
        	for (i = 0; i < nChild * 4 - 2; i++){
			 if(close (pipefds[i]) == -1) 
				ERROR ("CLOSE ERROR7");
		}
        	if (close (pipefds[nChild * 4 - 1]) == -1) ERROR("CLOSE ERROR8");
    	}
    	if (close (pipefds[nChild * 4]) == -1) 
		ERROR("CLOSE ERROR9");

    	if (nChild != nForks -1) for (i = nChild * 4 + 2; i < nForks * 4 + 1; i++) close (pipefds[i]);
    	return 0;
}

void child (int fd_prev, int fd_next)
{
    	void* buf = calloc (1024, sizeof(void));
    	int nBytes = -1;
    	while (nBytes) {
        	nBytes = read (fd_prev, buf, 1024);
        	if (write (fd_next, buf, nBytes) == -1)
		{ 
			ERROR("WRITE ERROR")
		}    
	}

    	free (buf);
}
