#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "http_server.h"
#include "http_request.h"
#include "http_response.h"
#include "sys_define.h"
#include "http_connection.h"

void daemonize()
{
	int i;
	int fd0;
	pid_t pid;	
	struct rlimit rl;
	struct sigaction sa;
	
	if((pid = fork()) > 0)
		exit(0);
	else if(pid < 0) {
		printf("fork failed: ");
		exit(0);
	}
	
	setsid();
	
	sa.sa_handler = SIG_IGN;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = 0;
	if(sigaction (SIGHUP, &sa, NULL) < 0) {
		perror("sigaction failed: ");
		exit(0);
	}

	if((pid = fork()) > 0)
		exit(0);
	else if(pid < 0) {
		printf("fork failed: ");
		exit(0);
	}

	umask(0);
	
	if(getrlimit (RLIMIT_NOFILE, &rl) < 0) { 
		perror ("getrlimit failed: ");
		exit(0);
	}

	if(rl.rlim_max == RLIM_INFINITY)
		rl.rlim_max = 1024;
	for(i = 0; i < rl.rlim_max; i++)
		close (i);
	
	fd0 = open("/dev/null", O_RDWR);
	dup2(fd0, STDIN_FILENO);
	dup2(fd0, STDOUT_FILENO);
	dup2(fd0, STDERR_FILENO);
}

int lockfile (int fd)
{
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    int result = (fcntl (fd, F_SETLK, &fl));

    return result;
}

int already_running ()
{
    char buf[20];
    int fd = open (LOCK_FILE, O_RDWR | O_CREAT, LOCK_MODE);

    if (fd < 0) {
		perror("open failed: ");
		exit(0);
	}

    if(lockfile(fd) < 0) {
		if (errno == EACCES || errno == EAGAIN) {
            close (fd);
            return L_HTTP_FAIL;
        }

		perror("lock file failed: ");
		exit(0); 
   	}

    if(ftruncate(fd, 0) < 0) {
		perror("fturncate failed: ");
		exit(0);
	}

    snprintf(buf, 20, "%ld", (long) getpid ());
    if(write(fd, buf, strlen (buf) + 1) < 0)
		return L_HTTP_FAIL;
	
    return L_HTTP_SUCCESS;
}

int main(int argc, char** argv)
{
	//daemonize();
	//if(already_running() == L_HTTP_FAIL)
	//	printf("arlready running");
	
	loop();	
  
    return 0;
}
