all:
	gcc -o trafficmgr trafficmgr.c monitor.c q.c -lpthread -pedantic-errors