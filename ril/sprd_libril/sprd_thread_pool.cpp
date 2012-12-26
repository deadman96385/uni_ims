/* //vendor/sprd/proprietories-source/ril/sprd_libril/sprd_thread_pool.cpp
 *
 * thread pool related interface implementation
 *
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 */

#include <pthread.h>
#include <time.h>
#include <errno.h>

#include <telephony/sprd_thread_pool.h>

threadpool_t *thread_pool_init(int max_threads, int idle_timeout)
{
	threadpool_t *threadpool;

	if (max_threads <= 0) {
		return NULL;
	}

	threadpool = (threadpool_t *)malloc(sizeof(threadpool_t));
	threadpool->queue_head = NULL;
	threadpool->thr_max = max_threads;
	threadpool->thr_alive = 0;
	threadpool->thr_queue = 0;
	threadpool->idle_timeout = idle_timeout;

	pthread_mutex_init(&(threadpool->pool_mutex), NULL);
	if (pthread_cond_init(&(threadpool->pool_cond), NULL) != 0) {
		free(threadpool);
		return NULL;
	}

	if (pthread_attr_init(&(threadpool->pool_attr)) != 0) {
		free(threadpool);
		return NULL;
	}

	if (pthread_attr_setdetachstate(&(threadpool->pool_attr), PTHREAD_CREATE_DETACHED) != 0) {
		free(threadpool);
		return NULL;
	}
	threadpool->state = POOL_VALID;

	return threadpool;
}

void thread_pool_destroy(threadpool_t *threadpool)
{
	if (!threadpool || (threadpool->state != POOL_VALID)) {
		return;
	}
  	if (pthread_mutex_lock(&threadpool->pool_mutex) != 0) {
   		ALOGE("Mutex lock failed!");
    		exit(-1);
	}
	threadpool->state = POOL_EXIT;

	/* wait for threads to exit */
	if (threadpool->thr_alive > 0) {
		if (pthread_cond_broadcast(&(threadpool->pool_cond)) != 0) {
			pthread_mutex_unlock(&threadpool->pool_mutex);
			return;
		}
	}
	while (threadpool->thr_alive > 0) {
		if (pthread_cond_wait (&threadpool->pool_cond, &threadpool->pool_mutex) != 0) {
			pthread_mutex_unlock(&threadpool->pool_mutex);
			return;
		}
	}
  	if (pthread_mutex_unlock(&threadpool->pool_mutex) != 0) {
    		ALOGE("Mutex unlock failed!");
    		exit(-1);
  	}

	pthread_mutex_destroy(&(threadpool->pool_mutex));
	pthread_cond_destroy(&(threadpool->pool_cond));
	pthread_attr_destroy(&(threadpool->pool_attr));
	free(threadpool);
	return;
}

void *thread_pool_worker(void *data)
{
	threadpool_t *threadpool = (threadpool_t *)data;
	thread_worker_t *worker = NULL;
	int retval = 0;
	int must_exit = 0;
	struct timespec timeout;

	/* loop looking for work */
	for (;;) {
		if (pthread_mutex_lock(&(threadpool->pool_mutex)) != 0) {
			/* Fatal error */
			ALOGE("!Fatal: mutex lock failed\n");
			exit(-1);
		}
		timeout.tv_sec = time(NULL) + threadpool->idle_timeout;
		timeout.tv_nsec = 0;
		threadpool->thr_queue++;
		while ((threadpool->queue_head == NULL) && (threadpool->state != POOL_EXIT)) {
			/* Sleep until be wakeup */
			retval = pthread_cond_timedwait(&(threadpool->pool_cond),
				&(threadpool->pool_mutex), &timeout);
			if (retval == ETIMEDOUT) {
				must_exit = 1;
				break;
			}
		}
		threadpool->thr_queue--;
		if (threadpool->state == POOL_EXIT) {
			must_exit = 1;
		}

		worker = threadpool->queue_head;
		if(worker)
			threadpool->queue_head = worker->next;
		else
			threadpool->queue_head = NULL;

		if (pthread_mutex_unlock(&(threadpool->pool_mutex)) != 0) {
			/* Fatal error */
			ALOGE("Fatal: mutex unlock failed!");
			exit(-1);
		}

		if (worker) {
			worker->handler(worker->data);
			free(worker);
			worker = NULL;
		} else if (must_exit) {
			break;
		}
	}

	if (pthread_mutex_lock(&(threadpool->pool_mutex)) != 0) {
		/* Fatal error */
		ALOGE("Fatal: mutex lock failed!");
		exit(-1);
	}
	threadpool->thr_alive--;
	if (threadpool->thr_alive == 0) {

		/* signal that all threads are finished */
		pthread_cond_broadcast(&threadpool->pool_cond);
	}
	if (pthread_mutex_unlock(&(threadpool->pool_mutex)) != 0) {
		/* Fatal error */
		ALOGE("Fatal: mutex unlock failed!");
		exit(-1);
	}

	return NULL;
}

int thread_pool_dispatch(threadpool_t *threadpool, void (*handler)(void *), void *user_data)
{
	pthread_t thr_id;
	thread_worker_t *new_worker = NULL;
	thread_worker_t *item = NULL;

	if (!threadpool) {
		return 0;
	}

	if (pthread_mutex_lock(&(threadpool->pool_mutex)) != 0) {
		ALOGE("Mutex lock failed!");
		return 0;
	}

	if (threadpool->state != POOL_VALID) {
		if (pthread_mutex_unlock(&(threadpool->pool_mutex)) != 0) {
			ALOGE("!Mutex unlock failed\n");
			return 0;
		}
		return 0;
	}

	new_worker = (thread_worker_t *)malloc(sizeof(thread_worker_t));
	new_worker->handler = handler;
	new_worker->data = user_data;
	new_worker->next = NULL;

	item = threadpool->queue_head;
	if (item == NULL) {
		threadpool->queue_head = new_worker;
	} else {
		while(item->next)
			item = item->next;
		item->next = new_worker;
	}

	if ((threadpool->thr_queue == 0) &&
			(threadpool->thr_alive < threadpool->thr_max)) {
		/* Creat a new thread */
		if (pthread_create(&thr_id, &(threadpool->pool_attr),
				thread_pool_worker, threadpool) != 0) {
			ALOGE("pthread_create failed!");
		} else {
			threadpool->thr_alive++;
		}
	}

	pthread_cond_signal(&(threadpool->pool_cond));

	if (pthread_mutex_unlock(&(threadpool->pool_mutex)) != 0) {
		ALOGE("Mutex unlock failed!");
		return 0;
	}
	return 1;
}

