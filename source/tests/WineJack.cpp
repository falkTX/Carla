/*
 * Wine+JACK Test
 * Copyright (C) 2015 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include <semaphore.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <windows.h>

#include <sys/types.h>
#include <jack/jack.h>

#undef _WIN32
#include <jack/thread.h>

static bool gCloseNow = false;

static void _close(int)
{
    gCloseNow = true;
}

static int _process(jack_nframes_t, void*)
{
    printf("%s\n", __PRETTY_FUNCTION__);
    return 0;
}

#if 0
// TEST
void* (*creator_func)(void*) = NULL;
void* creator_arg = NULL;
HANDLE creator_handle = 0;
pthread_t creator_pthread = 0;

static DWORD WINAPI thread_creator_helper(LPVOID)
{
    printf("%s\n", __PRETTY_FUNCTION__);
    creator_pthread = pthread_self();
    SetEvent(creator_handle);
    creator_func(creator_arg);
    return 0;
}

static int thread_creator(pthread_t* thread_id, const pthread_attr_t*, void *(*function)(void*), void* arg)
{
    printf("%s\n", __PRETTY_FUNCTION__);
    creator_func   = function;
    creator_arg    = arg;
    creator_handle = ::CreateEventA(NULL, false, false, NULL);

    ::CreateThread(NULL, 0, thread_creator_helper, arg, 0, 0);
    ::WaitForSingleObject(creator_handle, INFINITE);

    *thread_id = creator_pthread;
    return 0;
}
#endif

struct JackWineThread {
        void* (*func)(void*);
        void* arg;
        pthread_t pthid;
        sem_t sema;
};

static DWORD WINAPI
wine_thread_aux( LPVOID arg ) {
        struct JackWineThread* jwt = (struct JackWineThread*) arg;

    printf("%s\n", __PRETTY_FUNCTION__);

        void* func_arg = jwt->arg;
        void* (*func)(void*) = jwt->func;

        jwt->pthid = pthread_self();
        sem_post( &jwt->sema );

        func ( func_arg );

        return 0;
}

static int
wine_thread_create (pthread_t* thread_id, const pthread_attr_t*, void *(*function)(void*), void* arg) {
        struct JackWineThread jwt;

    printf("%s\n", __PRETTY_FUNCTION__);

        sem_init( &jwt.sema, 0, 0 );
        jwt.func = function;
        jwt.arg = arg;

        CreateThread( NULL, 0, wine_thread_aux, &jwt, 0, 0 );
        sem_wait( &jwt.sema );

        *thread_id = jwt.pthid;
        return 0;
}

int main()
{
    struct sigaction sterm;
    sterm.sa_handler  = _close;
    sterm.sa_flags    = SA_RESTART;
    sterm.sa_restorer = NULL;
    sigemptyset(&sterm.sa_mask);
    sigaction(SIGTERM, &sterm, NULL);
    sigaction(SIGINT,  &sterm, NULL);

    jack_set_thread_creator(wine_thread_create);

    jack_client_t* const client = jack_client_open("WineJack2", JackNullOption, NULL);
    if (client == NULL)
        return 1;

    jack_set_process_callback(client, _process, NULL);
    jack_activate(client);

    for (; ! gCloseNow;) {
        usleep(100000);
    }

    jack_deactivate(client);
    jack_client_close(client);

    return 0;
}
