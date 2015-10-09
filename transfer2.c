#include <pthread.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <semaphore.h>
#include "cbuf.c"

sem_t mutex_sem, occupied_sem, space_sem;

struct argument_t {
	FILE	*stream;
};

struct return_t {
	ssize_t	bytes;
};

#define ENTER_CRIT_SEC(syn_sem, mutex_sem)	\
{	\
	if (sem_wait(&syn_sem) == -1) {	/* P(syn_sem) */	\
		perror("sem_wait");	\
		_exit(2);	\
	}	\
	if (sem_wait(&mutex_sem) == -1) {	/* lock mutex */	\
		perror("sem_wait");	\
		_exit(2);	\
	}	\
}

#define OUT_CRIT_SEC(syn_sem, mutex_sem)	\
{	\
	if (sem_post(&mutex_sem) == -1) {	/* unlock mutex */	\
		perror("sem_post");	\
		_exit(2);	\
	}	\
	/* out of critical section */	\
	if (sem_post(&syn_sem) == -1) {	/* V(syn_sem) */	\
		perror("sem_post");	\
		_exit(2);	\
	}	\
}

static void *
read_func(void *arg)
{
	struct argument_t	*args;
	struct return_t	*res;	/* used for return bytes */
	FILE	*istream;
	size_t	len;
	ssize_t	read, size = 0;
	char	*line;

	res = malloc(sizeof(struct return_t));
	args = arg;
	istream = args->stream;
	
	while((read = getline(&line, &len, istream)) != -1) {
		ENTER_CRIT_SEC(space_sem, mutex_sem);
		/* enter critical section */
		/* have enough space to write to buffer */
		read = cbuf_copy_in(line);
		OUT_CRIT_SEC(occupied_sem, mutex_sem);
		printf("fill thread: wrote [%s] into buffer"
				" (nwrritten=%ld)\n", line, read);
		size += read;
	}
	/* write QUIT to buffer */
	ENTER_CRIT_SEC(space_sem, mutex_sem);
	read = cbuf_copy_in("QUIT");
	OUT_CRIT_SEC(occupied_sem, mutex_sem);
	printf("fill thread: wrote [%s] into buffer"
			" (nwrritten=%ld)\n", "QUIT", read);
	res->bytes = size;
	printf("Hello! res->bytes change normally!"
		"in read_func\n");
	return res;
}

static void *
write_func(void *arg)
{
	struct argument_t	*args;
	struct return_t	*res;	/* used for return bytes */
	FILE	*ostream;
	ssize_t	len, size = 0;
	char	*data = NULL;

	res = malloc(sizeof(struct return_t));
	data = calloc(CBUF_CAPACITY, 1);
	args = arg;
	ostream = args->stream;
	while (1) {
		/* fwrite(buffer, sizeof(char), size, ostream) */
		ENTER_CRIT_SEC(occupied_sem, mutex_sem);
		/* out of critical section */
		len = cbuf_copy_out(data);
		OUT_CRIT_SEC(space_sem, mutex_sem);
		printf("drain thread: read [%s]"
				" from buffer (nread=%ld)\n", data, len);
		/* case QIUT to break out */
		if (strcmp(data, "QUIT") == 0) {
			printf("Hello! i had cased QUIT!\n");
			break;
		}
		fwrite(data, sizeof(char), len, ostream);
		size += len;
	}
	res->bytes = size;
	printf("Hello! res->bytes change normally!"
		"in write_func\n");
	free(data);
	printf("Hello! free data normally!\n");
	return res;
}

int
main(int argc, char *argv[])
{
	pthread_t	r_tid, w_tid;
	struct argument_t	r_arg, w_arg;
	void	*r_res, *w_res;

	if (argc != 3) {
		perror("arguments");
		return EXIT_FAILURE;
	}

	if (sem_init(&mutex_sem, 0, 1) == -1) {	/* 1 for mutex */
		perror("sem_init");
		return EXIT_FAILURE;
	}
	if (sem_init(&occupied_sem, 0, 0) == -1) {	/* buffer initially empty */
		perror("sem_init");
		return EXIT_FAILURE;
	}
	if (sem_init(&space_sem, 0, 1) == -1) {	/* buffer can store 1 line */
		perror("sem_init");
		return EXIT_FAILURE;
	}
	if ((r_arg.stream = fopen(argv[1], "r")) == NULL) {
		perror("fopen");
		return EXIT_FAILURE;
	}
	if ((w_arg.stream = fopen(argv[2], "w")) == NULL) {
		perror("fopen");
		return EXIT_FAILURE;
	}

	cbuf_init();
	pthread_create(&r_tid, NULL,
					&read_func, &r_arg);
	pthread_create(&w_tid, NULL,
					&write_func, &w_arg);

	if (pthread_join(r_tid, &r_res) ||
		pthread_join(w_tid, &w_res)) {
		perror("pthread_join");
		return EXIT_FAILURE;
	}
	printf("Hello! pthreads exit normally!\n");
	if (sem_destroy(&mutex_sem) || 
		sem_destroy(&space_sem) ||
		sem_destroy(&occupied_sem)) {
		perror("sem_destroy");
		return EXIT_FAILURE;
	}

	fclose(r_arg.stream);
	fclose(w_arg.stream);
	cbuf_terminate();
	free(r_res);
	free(w_res);
	return 0;
}
