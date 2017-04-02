#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "thread.h"
#if defined WIN32 || defined _WIN32 || defined _WIN32
#include <windows.h>
#include <process.h>
void *thread_create(void (*thread_rout)(void *param), void *param)
{
        return (void *)_beginthread(thread_rout, 0, param);
}

void thread_kill(void *handle)
{
        TerminateThread(handle, 0);
}

void thread_sleep(int t)
{
        Sleep(t);
}

typedef struct win_event_t
{
        HANDLE handle;
} win_event_t;

event_t *thread_create_event()
{
        win_event_t *event = malloc(sizeof(win_event_t));

        event->handle = CreateEvent(NULL, FALSE, FALSE, NULL);

        return (event_t *)event;
}

void thread_set_event(event_t *_event)
{
        win_event_t *event = (win_event_t *)_event;

        SetEvent(event->handle);
}

void thread_reset_event(event_t *_event)
{
        win_event_t *event = (win_event_t *)_event;

        ResetEvent(event->handle);
}

int thread_wait_event(event_t *_event, int timeout)
{
        win_event_t *event = (win_event_t *)_event;

        if (timeout == -1)
                timeout = INFINITE;

        if (WaitForSingleObject(event->handle, timeout))
                return 1;
        return 0;
}

void thread_destroy_event(event_t *_event)
{
        win_event_t *event = (win_event_t *)_event;

        CloseHandle(event->handle);

        free(event);
}

#else
#include <pthread.h>

typedef struct event_pthread_t
{
	pthread_cond_t cond;
	pthread_mutex_t mutex;
} event_pthread_t;

thread_t *thread_create(void (*thread_rout)(void *param), void *param)
{
	pthread_t *thread = malloc(sizeof(pthread_t));
        
	pthread_create(thread, NULL, thread_rout, param);

	return thread;
}

void thread_kill(thread_t *handle)
{
	pthread_t *thread = (pthread_t *)handle;

	pthread_cancel(*thread);
	pthread_join(*thread, NULL);

	free(thread);
}

event_t *thread_create_event()
{
	event_pthread_t *event = malloc(sizeof(event_pthread_t));

	pthread_cond_init(&event->cond, NULL);
	pthread_mutex_init(&event->mutex, NULL);

        return (event_t *)event;
}

void thread_set_event(event_t *handle)
{
	event_pthread_t *event = (event_pthread_t *)handle;

	pthread_mutex_lock(&event->mutex);
	pthread_cond_broadcast(&event->cond);
	pthread_mutex_unlock(&event->mutex);
}

void thread_reset_event(event_t *handle)
{
}

int thread_wait_event(event_t *handle, int timeout)
{
	event_pthread_t *event = (event_pthread_t *)handle;
	struct timespec abstime;

	clock_gettime(CLOCK_REALTIME, &abstime);
	abstime.tv_nsec += (timeout % 1000) * 1000000;
	abstime.tv_sec += (timeout / 1000);
	if (abstime.tv_nsec > 1000000000)
	{
		abstime.tv_nsec -= 1000000000;
		abstime.tv_sec++;
	}

	pthread_mutex_lock(&event->mutex);
	if (timeout == -1)
		pthread_cond_wait(&event->cond, &event->mutex);
	else
		pthread_cond_timedwait(&event->cond, &event->mutex, &abstime);
	pthread_mutex_unlock(&event->mutex);

        return 0;
}

void thread_destroy_event(event_t *handle)
{
	event_pthread_t *event = (event_pthread_t *)handle;

	pthread_cond_destroy(&event->cond);
	pthread_mutex_destroy(&event->mutex);

	free(event);
}

void thread_sleep(int t)
{
	usleep(t * 1000);
}
#endif
