#ifndef __MONITOR_H__
#define __MONITOR_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "cart.h"
#include "q.h"

pthread_mutex_t	monitor;
pthread_cond_t	north_rail;
pthread_cond_t	west_rail;
pthread_cond_t	south_rail;
pthread_cond_t	east_rail;

extern void monitor_init();
extern void monitor_arrive(struct cart_t *);
extern void monitor_cross(struct cart_t *);
extern void monitor_leave(struct cart_t *);
extern void monitor_shutdown();

extern int get_right_dir(char dir);

#endif /* __MONITOR_H__ */
