#include "monitor.h"

void
lock_init(pthread_mutex_t *mutex)
{
	if (pthread_mutex_init(mutex, NULL) != 0) {
		perror("pthread_mutex_init");
		exit(EXIT_FAILURE);
	}
}

void
cond_init(pthread_cond_t *cond)
{
	if (pthread_cond_init(cond, NULL) != 0) {
		perror("pthread_cond_init");
		exit(EXIT_FAILURE);
	}
}

void
lock(pthread_mutex_t *mutex)
{
	if (pthread_mutex_lock(mutex) != 0) {
		perror("pthread_mutex_lock");
		exit(EXIT_FAILURE);
	}
}

void
unlock(pthread_mutex_t *mutex)
{
	if (pthread_mutex_unlock(mutex) != 0) {
		perror("pthread_mutex_unlock");
		exit(EXIT_FAILURE);
	}
}

void
wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	if (pthread_cond_wait(cond, mutex) != 0) {
		perror("pthread_cond_wait");
		exit(EXIT_FAILURE);
	}
}

void
signal(pthread_cond_t *cond)
{
	if (pthread_cond_signal(cond) != 0) {
		perror("pthread_cond_signal");
		exit(EXIT_FAILURE);
	}
}

void
mutex_destroy(pthread_mutex_t *mutex)
{
	if (pthread_mutex_destroy(mutex) != 0) {
		perror("pthread_mutex_destroy");
		exit(EXIT_FAILURE);
	}
}

void
cond_destroy(pthread_cond_t *cond) {
	if (pthread_cond_destroy(cond) != 0) {
		perror("pthread_cond_destroy");
		exit(EXIT_FAILURE);
	}
}

void
monitor_init()
{
	lock_init(&monitor);
	cond_init(&north_rail);
	cond_init(&west_rail);
	cond_init(&south_rail);
	cond_init(&east_rail);

	first = 0;	/* set no cart has been proceeded */
}

void
monitor_arrive(struct cart_t *cart)
{
	lock(&monitor);
	(void)fprintf(stderr, "Cart %d from %c arrives at intersection\n", 
							cart->num, cart->dir);
	
	while (first == 1 && next_cart != cart->dir) {
		(void)fprintf(stderr, "Cart %d from %c waiting before entering the intersection\n",
							cart->num, cart->dir);
		if (first != 0)
		switch (cart->dir) {
			case Q_NORTH:
				wait(&north_rail, &monitor);
				break;
			case Q_WEST:
				wait(&west_rail, &monitor);
				break;
			case Q_SOUTH:
				wait(&south_rail, &monitor);
				break;
			case Q_EAST:
				wait(&east_rail, &monitor);
				break;
		}
	}

	first = 1;	/* a cart has been proceeded */
	(void)fprintf(stderr, "Cart %d from %c allowed to proceed in to intersection\n",
							cart->num, cart->dir);
	unlock(&monitor);
}

void
monitor_cross(struct cart_t *cart)
{
	int i;

	lock(&monitor);

	(void)fprintf(stderr, "Cart %d from %c enter intersection\n",
							cart->num, cart->dir);

	/* cart entered intersection */
	q_cartHasEntered(cart->dir);

	(void)fprintf(stderr, "Cart %d from %c cross intersection\n",
							cart->num, cart->dir);
	(void)fprintf(stderr, "Time consumed: ");
	for (i = 0; i < TIME_PASS; i++) {
		(void)fprintf("%d sec..", i);
		sleep(1);
	}
	(void)fprintf(stderr, "%s\n", );
	unlock(&monitor);
}

void
monitor_leave(struct cart_t *cart)
{
	char	next_dir;

	lock(&monitor);
	(void)fprintf(stderr, "Cart %d from %c leave intersection\n",
							cart->num, cart->dir);

	for (next_dir = get_right_dir(cart->dir); next_dir != cart->dir; next_dir = get_right_dir(next_dir)) {
		(void)fprintf(stderr, "Try to signal %c thread to proceed intersection\n",
								next_dir);
		if (q_cartIsWaiting(next_dir)) {
			next_cart = next_dir;

			switch (next_dir) {
				case Q_NORTH:
					signal(&north_rail);
					break;
				case Q_WEST:
					signal(&west_rail);
					break;
				case Q_SOUTH:
					signal(&south_rail);
					break;
				case Q_EAST:
					signal(&east_rail);
					break;
			}
		}
	}

	/* no cart in queues set first to 0 */
	first = 0;

	unlock(&monitor);
}

void
monitor_shutdown()
{
	mutex_destroy(&monitor);
	cond_destroy(&north_rail);
	cond_destroy(&west_rail);
	cond_destroy(&south_rail);
	cond_destroy(&east_rail);
}