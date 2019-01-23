#include "threadpool.h"

#define DEBUG

/*
 *initializing thread pool
 */
void thread_pool_init(thread_pool* pool,int max_thread_size)
{
	//initialize mutex
	pthread_mutex_init(&(pool->mutex),NULL);
	pthread_cond_init(&(pool->cond),NULL);
	pool->runner_head = NULL;
	pool->runner_tail = NULL;
	pool->max_thread_size = max_thread_size;
	pool->shutdown = 0;

	//create seperate threadpool
	pool->threads = (pthread_t*)malloc(max_thread_size*sizeof(pthread_t));
	for(int i = 0;i<max_thread_size;i++)
	{
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
		pthread_create(&(pool->threads[i]),&attr,(void*)run,(void*)pool);
	}
#ifdef DEBUG
	printf("thread_pool_create %d detached thread\n",max_thread_size);
#endif
}

void run(void* arg)
{
	thread_pool* pool = (thread_pool*)arg;
	while(1)
	{
		//lock
		pthread_mutex_lock(&(pool->mutex));
#ifdef DEBUG
		//printf("run->lock\n");
#endif
		//if waiting queue = 0 and pool is not destroyed -> conjunction
		while(pool->runner_head == NULL&&!pool->shutdown)
		{
			pthread_cond_wait(&(pool->cond),&(pool->mutex));
		}

		//if pools is destroyed
		if(pool->shutdown)
		{
			//unlock
			pthread_mutex_unlock(&(pool->mutex));
#ifdef DEBUG
			printf("run->lock and thread exit\n");
#endif
			pthread_exit(NULL);	
		}

		//get first mission
		thread_runner* runner = pool->runner_head;
		pool->runner_head = runner->next;
		pthread_mutex_unlock(&(pool->mutex));
#ifdef DEBUG
		printf("run->unlock\n");
#endif
		//execute mission
		(runner->callback)(runner->arg);
		free(runner);
		runner = NULL;
#ifdef DEBUG
		printf("run->runned and free runner\n");
#endif
	}
	pthread_exit(NULL);
}

/*
 	add new mission to pool
 */
void threadpool_add_runner(thread_pool* pool,void*(*callback)(void* arg),void* arg)
{
	//create new mission
	thread_runner* newrunner = (thread_runner*)malloc(sizeof(thread_runner));
	newrunner->callback = callback;
	newrunner->arg = arg;
	newrunner->next = NULL;

	//lock on, also push mission to waiting queue
	pthread_mutex_lock(&(pool->mutex));
#ifdef DEBUG
	printf("threadpool_add_runner->locked\n");
#endif
	if(pool->runner_head != NULL)
	{
		pool->runner_tail->next = newrunner;
		pool->runner_tail = newrunner;
	}
	else
	{
		pool->runner_head = newrunner;
		pool->runner_tail = newrunner;
	}
	pthread_mutex_unlock(&(pool->mutex));
#ifdef DEBUG
	printf("threadpool_add_runner->unlocked\n");
#endif
	//wake up waiting queue
	pthread_cond_signal(&(pool->cond));
#ifdef DEBUG
	printf("threadpool_add_runner->add a runner and wakeip a waiting thread\n");
#endif
}

/*
	destroy pools
 */
void thread_destroy(thread_pool** ppool)
{
	thread_pool* pool = *ppool;
	//prevent from second destroy
	if(!pool->shutdown)
	{
		pool->shutdown = 1;
		//awake every waiting queue
		pthread_cond_broadcast(&(pool->cond));
		sleep(1);
#ifdef DEBUG 
		printf("threadpool_destroy->wakeup all waiting thread\n");
#endif
		//reuse the threads
		free(pool->threads);
		//destroy waiting queue
		thread_runner *head = NULL;
		while(pool->runner_head != NULL)
		{
			head = pool->runner_head;
			pool->runner_head = pool->runner_head->next;
			free(head);
		}
#ifdef DEBUG
		printf("threadpool_destroy->all runner freed\n");
#endif
		pthread_mutex_destroy(&(pool->mutex));
		pthread_cond_destroy(&(pool->cond));
#ifdef DEBUG
		printf("threadpool_destroy->mutex and cond destoryed\n");
#endif
		free(pool);
		(*ppool) = NULL;
#ifdef DEBUG
		printf("threadpool_destroy->pool freed\n");
#endif
	}
}