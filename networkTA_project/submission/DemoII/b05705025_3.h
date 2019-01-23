#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct runner
{
	void* (*callback)(void*arg);
	void* arg;
	struct runner *next;
}thread_runner;


typedef struct 
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	//the head pointer of all queue pool
	thread_runner* runner_head;
	//the tail pointer of all queue pool
	thread_runner* runner_tail;
	//every threads
	pthread_t* threads;
	//the maximum amount of threads
	int max_thread_size;
	//if pool shutdown
	int shutdown;
}thread_pool;

/*
 execution
 */
void run(void* arg);

/*
 *inicialization
parameter
pool：pointer that's point to avalible pool
 */
void thread_pool_init(thread_pool* pool,int max_thread_size);

void threadpool_add_runner(thread_pool* pool,void*(*callback)(void* arg),void* arg);

/*
 	destroy pools
 */
void thread_destroy(thread_pool** ppool);

#endif