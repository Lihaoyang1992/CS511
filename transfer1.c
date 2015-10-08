#include "cbuf.c"

pthread_mutex_t lock

struct argument_t {
	char	*str;
};

struct return_t {
	int	bytes;
};

static void *
read_func(void *arg)
{
	struct argument_t	*args;
	struct return_t	*res;
	int i;
	char	*str;

	res = malloc(sizeof(struct return_t));
	args = arg;
	str = args->str;
	cbuf_copy_in(buffer);
}