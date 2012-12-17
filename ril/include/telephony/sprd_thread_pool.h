/* //vendor/sprd/proprietories-source/ril/include/telephony/sprd_thread_pool.h
 *
 * thread pool related interface
 *
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 */

#ifndef __THRMGR_H__
#define __THRMGR_H__ 1

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <sys/time.h>
#include <utils/Log.h>
#include <stdlib.h>

typedef struct thread_worker {
	void (*handler)(void *data);
	void *data;
	struct thread_worker *next;
} thread_worker_t;

typedef enum pool_state {
	POOL_INVALID,
	POOL_VALID,
	POOL_EXIT,
} pool_state_t;

typedef struct threadpool {
	pthread_mutex_t pool_mutex;
	pthread_cond_t pool_cond;
	pthread_attr_t pool_attr;

	pool_state_t state;
	int thr_max;
	int thr_queue;
	int thr_alive;
	int idle_timeout;

	thread_worker_t *queue_head;
} threadpool_t;

threadpool_t *thread_pool_init(int max_threads, int idle_timeout);
void thread_pool_destroy(threadpool_t *threadpool);
int thread_pool_dispatch(threadpool_t *threadpool, void (*handler)(void *), void *user_data);

#ifdef __cplusplus
}
#endif

#endif


