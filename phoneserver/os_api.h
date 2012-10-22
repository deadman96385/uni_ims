/*
 *
 * os_api.h: os api  implementation for the phoneserver

 *Copyright (C) 2009,  spreadtrum
 *
 * Author: jim.cui <jim.cui@spreadtrum.com.cn>
 *
 */

#ifndef  _OS_API_H
#define   _OS_API_H

#include <pthread.h>
#include<semaphore.h>
#include <sched.h>
#include "debug.h"
#include <sys/time.h>
#include <time.h>

#define  mutex 						pthread_mutex_t
#define  sem	 					       sem_t
#define  cond						       pthread_cond_t

#define mutex_init(mutex,attr)  		pthread_mutex_init(mutex,attr)
#define sem_init(mutex,attr,value)  	sem_init(mutex,attr,value)
#define cond_init(mutex,attr)			pthread_cond_init(mutex,attr)

#define  thread_t    					pthread_t
#define thread_creat 					pthread_create

#define mutex_lock					pthread_mutex_lock

#define mutex_unlock 				pthread_mutex_unlock

#define mutex_try_lock 				sem_trywait

#define sem_lock						sem_wait

#define sem_unlock	 				sem_post

#define sem_try_lock 				sem_trywait

#define thread_sched_param                struct sched_param

#define thread_setschedparam	       pthread_setschedparam

#define thread_getschedparam             pthread_getschedparam

#define thread_cond_t				 pthread_cond_t
#define thread_cond_timedwait              pthread_cond_timedwait
#define thread_cond_wait			        pthread_cond_wait
#define thread_cond_signal			 pthread_cond_signal

#define AT_DEBUG 0

#if AT_DEBUG
extern void AT_DUMP(const char *prefix, const char *buff, int len);

#else /*  */
#define  AT_DUMP(prefix,buff,len)  do{}while(0)
#endif /*  */

#endif /*  */
