#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include "monitor.h"
#include "q.h"
#include "cart.h"

#define DIR	4

struct thread_arg
{
	int	direction;
};

void *
cart_thread(void *args)
{
	struct thread_arg	*arg = (struct thread_arg *)args;
	struct cart_t	*cart;
	int	direction = arg->direction;

	fprintf(stderr, "thread for direction %c starts\n", direction);
	cart = q_getCart(direction);
	while (cart != NULL) {
		fprintf(stderr, "thread for direction %c gets cart %i\n", 
												direction, cart->num);
		monitor_arrive(cart);
		monitor_cross(cart);
		monitor_leave(cart);
		cart = q_getCart(direction);
	}
	fprintf(stderr, "thread for direction %c exits\n", direction);

	return NULL;
}

int
check_input(char input)
{
	if (input == Q_NORTH ||
		input == Q_WEST ||
		input == Q_SOUTH ||
		input == Q_EAST) {
		return 1;
	}

	return 0;
}

int
get_right_dir(char dir)
{
	if (dir == Q_NORTH)
		return Q_WEST;
	else if (dir == Q_WEST)
		return Q_SOUTH;
	else if (dir == Q_SOUTH)
		return Q_EAST;
	else if (dir == Q_EAST)
		return Q_NORTH;

	return -1;
}

void
usage()
{
	(void)fprintf(stderr, "Usage: ./trafficmgr <carts queues> (only contains [n|w|e|s])\n");
}

int
main(int argc, char *argv[])
{
	int	i;
	int	ret_thread;
	pthread_t	pid[DIR];
	struct thread_arg	thread_args[DIR];

	if (argc != 2) {
		usage();
		return EXIT_FAILURE;
	}

	for (i = 0; argv[1][i] != '\0'; i++) {
		if (check_input(argv[1][i]) == 0) {
			usage();
			return EXIT_FAILURE;
		}
	}

	/* initialize monitors and queues */
	q_init();
	monitor_init();

	/* put carts into the queues */
	for (i = 0; argv[1][i] != '\0'; i++)
		q_putCart(argv[1][i]);

	q_print(Q_NORTH);
	q_print(Q_WEST);
	q_print(Q_SOUTH);
	q_print(Q_WEST);

	for (i = 0; i < DIR; i++) {
		char dir;

		if (i == 0)
			dir = Q_NORTH;
		else if (i == 1)
			dir = Q_WEST;
		else if (i == 2)
			dir = Q_SOUTH;
		else
			dir = Q_EAST;

		thread_args[i].direction = dir;

		if ((ret_thread = pthread_create(&pid[i], NULL, cart_thread, &thread_args[i])) != 0) {
			perror("pthread_create");
			return EXIT_FAILURE;
		}
	}

	for (i = 0; i < DIR; i++) {
		if ((ret_thread = pthread_join(pid[i], NULL)) != 0) {
			perror("pthread_join");
			return EXIT_FAILURE;
		}
	}
	/*
	for (i = 0; i < DIR; i++) {
		if (i == 0)
			(void)fprintf(stderr, "north queue has %d carts\n",
							q_cartIsWaiting(Q_NORTH));
		else if (i == 1)
			(void)fprintf(stderr, "west queue has %d carts\n",
							q_cartIsWaiting(Q_WEST));
		else if (i == 2)
			(void)fprintf(stderr, "south queue has %d carts\n",
							q_cartIsWaiting(Q_SOUTH));
		else if (i == 3)
			(void)fprintf(stderr, "east queue has %d carts\n",
							q_cartIsWaiting(Q_EAST));
	}
	*/
	monitor_shutdown();
	q_shutdown();
	return 0;
}
