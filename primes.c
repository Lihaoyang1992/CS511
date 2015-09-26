#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define CHECK_SUCCESS 0
#define CHECK_FAILURE 1
#define FIFO_PATH "/tmp/myfifo"
#define FIFO_MODE 0666

void usage(void)
{
	fprintf(stderr, "usage: ./primes <increasing positive integers>\n");
}
int check_arg(int argc, char** argv)
{
	if (argc < 2){
		usage();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
int wait_error_check(int pid, int primes_num) {
	int w, status;
	w = waitpid(pid, &status, WUNTRACED | WCONTINUED);
	if (w == -1) {
		perror("Error: can't wait child");
	}
	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status) == primes_num)
			printf("child %d exited correctly\n", pid);
	} else if (WIFSIGNALED(status)) {
		printf("killed by sigal %d\n", WTERMSIG(status));
	} else if (WIFSTOPPED(status)) {
		printf("stopped by signal %d\n", WSTOPSIG(status));
	} else if (WIFCONTINUED(status)) {
		printf("continued\n");
	}
	else {
		return CHECK_FAILURE;
	}
	return CHECK_SUCCESS;
}
int find_and_submit_primes(int wfd, int bot, int top) {
	int flag = 0;
	int primes_num = 0;
	if (top < 2)
		return CHECK_SUCCESS;
	if (bot == 2)
	{
		if (write(wfd, &bot, sizeof(bot)) == -1) {
			perror("child can't write");
			return EXIT_FAILURE;
		}
		primes_num++;
	}
	if ((bot & 1) == 0)
		bot++;
	int i;
	for (i = bot; i <= top; i += 2)
	{
		flag = 0;
		int j;
		for (j = 2; j <= i / 2; j++)
		{
			if ((i % j) == 0)
			{
				flag = 1;
				break;
			}
		}
		if (flag == 0) {
			if (write(wfd, &i, sizeof(i)) == -1) {
				perror("child can't write");
				return EXIT_FAILURE;
			}
			primes_num++;
		}
	}
	return primes_num;
}
int write_to_fd(const int wfd, int bot, int top)
{
	return find_and_submit_primes(wfd, bot, top);
}

int main(int argc, char *argv[]) {
	if (check_arg(argc, argv) != EXIT_SUCCESS)
		return EXIT_FAILURE;
	/* pipe used by odd child */
	int *pipe_fds = (int *)malloc(sizeof(int) * (argc - 1));
	/* char* used by even child */
	char* myfifo;
	
	/* all file descriptor hold by parent */
	int *parent_fds = (int *)malloc(sizeof(int) * (argc - 1));
	/* fd used by child to write */
	int write_fd;
	int max_fd;
	
	//int child_pid[argc - 1];
	int *child_pid = (int *)malloc(sizeof(int) * (argc - 1));
	pid_t parent_id = getpid();
	
	fd_set call_set;

	int bot, top, i, num, prime;

	for (i = 0; i < argc - 1; ++i) {
		if ((i & 1) == 0) {
			/* odd child create pipe */
			int *pipe_fd = pipe_fds + i;
			pipe(pipe_fd);
			parent_fds[i] = pipe_fd[0];
			write_fd = pipe_fd[1];
		}
		else {
			/* even child create fifo */
			myfifo = (char *)malloc(sizeof(FIFO_PATH));
			sprintf(myfifo, "%s%2d", FIFO_PATH, i);
			
			if (mkfifo(myfifo, FIFO_MODE) == -1)
				perror("make fifo error");
		}
		child_pid[i] = fork();
		if (child_pid[i]  == -1) {
			perror("fork error");
			return EXIT_FAILURE;
		}
		if ( (i & 1) == 0 ) {
			/* parent close write fd of odd child */
			if (getpid() == parent_id)
				close(write_fd);
		}
		else {
			/* even and parent open fifo child */
			if (getpid() != parent_id) {
				/* child */
				if ((write_fd = open(myfifo, O_WRONLY)) == -1)
					perror("child open fifo error");
				unlink(myfifo);
				free(myfifo);
			}
			else {
				/* parent */
				if ((parent_fds[i] = open(myfifo, O_RDONLY)) == -1)
					perror("parent open fifo eror");
			}
		}
		if (child_pid[i] == 0) {
			bot = 2;
			top = (atoi)(argv[i + 1]);
			if (i != 0)
				bot = (atoi)(argv[i]) + 1;
			printf("child %d: bottom=%d, top=%d\n", getpid(), bot, top);
			break;
		}
	}

	if (getpid() == parent_id) {
		/* PARENT SECTION */
		int* primes_num = (int *)malloc(sizeof(int) * (argc - 1));
		for (i = 0; i < argc - 1; ++i)
			primes_num[i] = 0;
		int set_fd_num, deleted_num = 0;
		/* parent use select(3) read data from fds */
		for(;;) {
			FD_ZERO(&call_set);
			for (i = 0; i < argc - 1; ++i) {
				if (parent_fds[i] != -1) {
					FD_SET(parent_fds[i], &call_set);
					set_fd_num = i;
				}
			}
			max_fd = parent_fds[set_fd_num];
			if ( (num = select(max_fd + 1, &call_set, NULL, NULL, NULL)) < 0 ) {
				perror("select error");
				return EXIT_FAILURE;
			}
			for (i = 0; i <= max_fd; ++i) {
				if (FD_ISSET(i, &call_set)) {
					int read_status = read(i, &prime, sizeof(prime));
					if ( read_status == -1) {
						perror("read fd error");
						return EXIT_FAILURE;
					}
					else if (read_status > 0) {
						primes_num[i]++;
						printf("%d is prime\n", prime);
					}
					else{
						/* delete fd i in parent_fds */
						int j;
						for (j = 0; j < argc - 1; ++j) {
							if (parent_fds[j] == i) {
								parent_fds[j] = -1;
								deleted_num++;
								break;
							}
						}
						/*  wait child return and check the return value*/
						if (wait_error_check(child_pid[j], primes_num[i]) == EXIT_FAILURE) {
							perror("child terminated error, can't find information");
							return EXIT_FAILURE;
						}
						
					}
				}
			}
			if (deleted_num == argc - 1)
				break;
		}
	}
	else if ((i & 1) == 0) {
		/* ODD CHILD SECTION */
		/* let i be the real index */
		int primes_num;
		close(parent_fds[i]);
		//printf("fd is %d\n", write_fd);
		primes_num = write_to_fd(write_fd, bot, top);
		close(write_fd);

		return primes_num;
	}
	else {
		/* EVEN CHILD SECTION */
		int primes_num;
		//printf("fd is %d\n", write_fd);
		primes_num = write_to_fd(write_fd, bot, top);
		return primes_num;
	}
	return EXIT_SUCCESS;
}