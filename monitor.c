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
mutex_destory(pthread_mutex_t *mutex)
{
	if (pthread_mutex_destory(mutex) != 0) {
		perror("pthread_mutex_destory");
		exit(EXIT_FAILURE);
	}
}

void
cond_destory(pthread_cond_t *cond) {
	if (pthread_cond_destory(cond) != 0) {
		perror("pthread_cond_destory");
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
}

void
monitor_arrive(struct cart_t *cart)
{
	lock(&monitor);
	(void)fprintf(stderr, "Cart %d from %c arrives at intersection\n", 
							cart->num, cart->dir);
	while (next_cart != cart->dir) {
		(void)fprintf(stderr, "Cart %d from %c waiting before entering the intersection\n",
							cart->num, cart->dir);
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

	(void)fprintf(stderr, "Cart %d from %c enter intersection\n",
							cart->num, cart->dir);

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
		if (q_cartIsWaiting(next_dir)) {
			switch (next_dir) {
				case Q_NORTH:
					signal(&north_rail);
				case Q_WEST:
					signal(&west_rail);
				case Q_SOUTH:
					signal(&south_rail);
				case Q_EAST:
					signal(&east_rail);			
			}
		}

		(void)fprintf(stderr, "Signal %c thread to proceed intersection\n",
								next_dir);
	}

	unlock(&monitor);
}

void
monitor_shutdown()
{
	mutex_destory(&monitor);
	cond_destory(&north_rail);
	cond_destory(&west_rail);
	cond_destory(&south_rail);
	cond_destory(&east_rail);
}