#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>

/* Test function */
void testfunction(void)
{
    printf("TestFUNCTION\n");
}

/* Mutex operations */
int th_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
    return pthread_mutex_init(mutex, attr);
}

int th_mutex_lock(pthread_mutex_t *mutex)
{
    return pthread_mutex_lock(mutex);
}

int th_mutex_unlock(pthread_mutex_t *mutex)
{
    return pthread_mutex_unlock(mutex);
}

/* Thread creation and management */
int th_create(pthread_t *thread, const pthread_attr_t *attr,
              void *(*start_routine)(void *), void *arg)
{
    return pthread_create(thread, attr, start_routine, arg);
}

int th_detach(pthread_t thread)
{
    return pthread_detach(thread);
}

int th_join(pthread_t thread, void **thread_return)
{
    return pthread_join(thread, thread_return);
}

void th_exit(void *retval)
{
    pthread_exit(retval);
}

int th_kill(pthread_t thread, int signo)
{
    return pthread_kill(thread, signo);
}

pthread_t th_self(void)
{
    return pthread_self();
}

int th_cancel(pthread_t thread)
{
    return pthread_cancel(thread);
}
