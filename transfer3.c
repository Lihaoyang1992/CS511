#include <pthread.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include "cbuf.c"

#define SUSPEND_READ	1
#define SUSPEND_WRITE	1000

static pthread_mutex_t lock;
static FILE	*istream, *ostream;

struct argument_t {
	FILE	*stream;
};

static void *
read_func(void *arg)
{
	size_t	len;
	ssize_t	read, size = 0;
	char	*line;
	int	*retval;

	while((read = getline(&line, &len, istream)) != -1) {
		while (1) {
			if (usleep(SUSPEND_READ)) {
				perror("usleep");
				_exit(2);
			}
			pthread_mutex_lock(&lock);	
			/* enter critical section */
			if (cbuf_space_available() >= strlen(line)) {
			/* have enough space to write to buffer */
				break;
			}
			pthread_mutex_unlock(&lock);
			/* out of critical section */
			printf("fill thread: could not write [%s]"
					" -- not enough space\n", line);
		}
		read = cbuf_copy_in(line);
		pthread_mutex_unlock(&lock);
		printf("fill thread: wrote [%s] into buffer"
				" (nwrritten=%ld)\n", line, read);
		size += read;
	}
	while (1) {
		if (usleep(SUSPEND_READ)) {
			perror("usleep");
			_exit(2);
		}
		pthread_mutex_lock(&lock);	/* write QUIT to buffer */
		if (cbuf_space_available() >= strlen("QUIT"))
			break;
		pthread_mutex_unlock(&lock);
	}
	read = cbuf_copy_in("QUIT");
	pthread_mutex_unlock(&lock);
	printf("fill thread: wrote [%s] into buffer"
			" (nwrritten=%ld)\n", "QUIT", read);
	
	retval = (int *)malloc(sizeof(int));
	*retval = size;
	return (void *)retval;
}

static void *
write_func(void *arg)
{
	ssize_t	len, size = 0;
	char	*data = NULL;
	int	*retval;

	data = malloc(CBUF_CAPACITY * sizeof(char));

	/* fwrite(buffer, sizeof(char), size, ostream) */
	while (1) {
		while(1) {
			if (usleep(SUSPEND_WRITE)) {
				perror("usleep");
				_exit(2);
			}
			pthread_mutex_lock(&lock);
			/* enter critical section */
			if (cbuf_data_is_available())
				break;
			pthread_mutex_unlock(&lock);
			/* out of critical section */
			printf("drain thread: no new string in buffer\n");
		}
		len = cbuf_copy_out(data);
		/*printf("len is %zd\n", len);*/
		pthread_mutex_unlock(&lock);
		size += len;
		printf("drain thread: read [%s]"
				" from buffer (nread=%ld)\n", data, len);
		if (strcmp(data, "QUIT") == 0)
			break;
		fwrite(data, sizeof(char), len, ostream);
	}
	
	free(data);
	retval = (int *)malloc(sizeof(int));
	*retval = size;
	return (void *)retval;
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

	if ((istream = fopen(argv[1], "r")) == NULL) {
		perror("fopen");
		return EXIT_FAILURE;
	}
	if ((ostream = fopen(argv[2], "w")) == NULL) {
		perror("fopen");
		return EXIT_FAILURE;
	}

	cbuf_init();
	pthread_create(&r_tid, NULL, &read_func, NULL);
	pthread_create(&w_tid, NULL, &write_func, NULL);

	if (pthread_join(r_tid, &r_res) ||
		pthread_join(w_tid, &w_res)) {
		perror("pthread_join");
		return EXIT_FAILURE;
	}

	fclose(r_arg.stream);
	fclose(w_arg.stream);
	cbuf_terminate();
	free(r_res);
	free(w_res);
	return 0;
}
