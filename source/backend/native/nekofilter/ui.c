/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
 *
 * LV2 UI bundle shared library for communicating with a DSSI UI
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 *****************************************************************************/

#define UI_EXECUTABLE "nekofilter-ui"

#define WAIT_START_TIMEOUT  3000 /* ms */
#define WAIT_ZOMBIE_TIMEOUT 3000 /* ms */
#define WAIT_STEP 100            /* ms */

//#define FORK_TIME_MEASURE

#define USE_VFORK
//#define USE_CLONE
//#define USE_CLONE2

#if defined(USE_VFORK)
#define FORK vfork
#define FORK_STR "vfork"
#elif defined(USE_CLONE)
#define FORK_STR "clone"
#elif defined(USE_CLONE2)
#define FORK_STR "clone2"
#else
#define FORK fork
#define FORK_STR "fork"
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#if defined(FORK_TIME_MEASURE)
# include <sys/time.h>
#endif
#include <unistd.h>
#if defined(USE_CLONE) || defined(USE_CLONE2)
# include <sched.h>
#endif
#include <fcntl.h>
#include <locale.h>
#include <errno.h>

#include "CarlaNative.h"

struct control
{
  HostDescriptor* host;

  bool running;              /* true if UI launched and 'exiting' not received */
  bool visible;              /* true if 'show' sent */

  int send_pipe;             /* the pipe end that is used for sending messages to UI */
  int recv_pipe;             /* the pipe end that is used for receiving messages from UI */

  pid_t pid;
};

static
char *
read_line(
  struct control * control_ptr)
{
  ssize_t ret;
  char ch;
  char buf[100];
  char * ptr;

  ptr = buf;

loop:
  ret = read(control_ptr->recv_pipe, &ch, 1);
  if (ret == 1 && ch != '\n')
  {
    *ptr++ = ch;
    goto loop;
  }

  if (ptr != buf)
  {
    *ptr = 0;
    //printf("recv: \"%s\"\n", buf);
    return strdup(buf);
  }

  return NULL;
}

static
bool
wait_child(
  pid_t pid)
{
  pid_t ret;
  int i;

  if (pid == -1)
  {
    fprintf(stderr, "Can't wait for pid -1\n");
    return false;
  }

  for (i = 0; i < WAIT_ZOMBIE_TIMEOUT / WAIT_STEP; i++)
  {
    //printf("waitpid(%d): %d\n", (int)pid, i);

    ret = waitpid(pid, NULL, WNOHANG);
    if (ret != 0)
    {
      if (ret == pid)
      {
        //printf("child zombie with pid %d was consumed.\n", (int)pid);
        return true;
      }

      if (ret == -1)
      {
        fprintf(stderr, "waitpid(%d) failed: %s\n", (int)pid, strerror(errno));
        return false;
      }

      fprintf(stderr, "we have waited for child pid %d to exit but we got pid %d instead\n", (int)pid, (int)ret);

      return false;
    }

    //printf("zombie wait %d ms ...\n", WAIT_STEP);
    usleep(WAIT_STEP * 1000);   /* wait 100 ms */
  }

  fprintf(
    stderr,
    "we have waited for child with pid %d to exit for %.1f seconds and we are giving up\n",
    (int)pid,
    (float)((float)WAIT_START_TIMEOUT / 1000));

  return false;
}

void
nekoui_run(
  struct control * control_ptr)
{
  char * msg;
  char * port_index_str;
  char * port_value_str;
  int index;
  float value;
  char * locale;

  msg = read_line(control_ptr);
  if (msg == NULL)
  {
    return;
  }

  locale = strdup(setlocale(LC_NUMERIC, NULL));
  setlocale(LC_NUMERIC, "POSIX");

  if (!strcmp(msg, "port_value"))
  {
    port_index_str = read_line(control_ptr);
    port_value_str = read_line(control_ptr);

    index = atoi(port_index_str);
    if (sscanf(port_value_str, "%f", &value) == 1)
    {
      //printf("port %d = %f\n", port, value);
      control_ptr->host->ui_parameter_changed(control_ptr->host->handle, index, value);
    }
    else
    {
      fprintf(stderr, "failed to convert \"%s\" to float\n", port_value_str);
    }

    free(port_index_str);
    free(port_value_str);
  }
  else if (!strcmp(msg, "close"))
  {
    control_ptr->visible = false;
    control_ptr->host->ui_closed(control_ptr->host->handle);
  }
  else if (!strcmp(msg, "exiting"))
  {
    /* for a while wait child to exit, we dont like zombie processes */
    if (!wait_child(control_ptr->pid))
    {
      fprintf(stderr, "force killing misbehaved child %d (exit)\n", (int)control_ptr->pid);
      if (kill(control_ptr->pid, SIGKILL) == -1)
      {
        fprintf(stderr, "kill() failed: %s (exit)\n", strerror(errno));
      }
      else
      {
        wait_child(control_ptr->pid);
      }
    }

    control_ptr->running = false;
    control_ptr->visible = false;
    control_ptr->host->ui_closed(control_ptr->host->handle);
  }
  else
  {
    printf("unknown message: \"%s\"\n", msg);
  }

  setlocale(LC_NUMERIC, locale);
  free(locale);

  free(msg);
}

void
nekoui_show(
  struct control * control_ptr)
{
  if (control_ptr->visible)
  {
    return;
  }

  write(control_ptr->send_pipe, "show\n", 5);
  control_ptr->visible = true;
}

void
nekoui_hide(
  struct control * control_ptr)
{
  if (!control_ptr->visible)
  {
    return;
  }

  write(control_ptr->send_pipe, "hide\n", 5);
  control_ptr->visible = false;
}

void
nekoui_quit(
  struct control * control_ptr)
{
  write(control_ptr->send_pipe, "quit\n", 5);
  control_ptr->visible = false;

  /* for a while wait child to exit, we dont like zombie processes */
  if (!wait_child(control_ptr->pid))
  {
    fprintf(stderr, "force killing misbehaved child %d (exit)\n", (int)control_ptr->pid);
    if (kill(control_ptr->pid, SIGKILL) == -1)
    {
      fprintf(stderr, "kill() failed: %s (exit)\n", strerror(errno));
    }
    else
    {
      wait_child(control_ptr->pid);
    }
  }
}

#if defined(FORK_TIME_MEASURE)
static
uint64_t
get_current_time()
{
   struct timeval time;

   if (gettimeofday(&time, NULL) != 0)
       return 0;

   return (uint64_t)time.tv_sec * 1000000 + (uint64_t)time.tv_usec;
}

#define FORK_TIME_MEASURE_VAR_NAME  ____t

#define FORK_TIME_MEASURE_VAR       uint64_t FORK_TIME_MEASURE_VAR_NAME
#define FORK_TIME_MEASURE_BEGIN     FORK_TIME_MEASURE_VAR_NAME = get_current_time()
#define FORK_TIME_MEASURE_END(msg)                                                       \
  {                                                                                      \
    FORK_TIME_MEASURE_VAR_NAME = get_current_time() - FORK_TIME_MEASURE_VAR_NAME;        \
    fprintf(stderr, msg ": %llu us\n", (unsigned long long)FORK_TIME_MEASURE_VAR_NAME);  \
  }

#else

#define FORK_TIME_MEASURE_VAR
#define FORK_TIME_MEASURE_BEGIN
#define FORK_TIME_MEASURE_END(msg)

#endif

#if defined(USE_CLONE) || defined(USE_CLONE2)

static int clone_fn(void * context)
{
    execvp(*(const char **)context, (char **)context);
    return -1;
}

#endif

static
struct control*
nekoui_instantiate(
  HostDescriptor* host)
{
  struct control * control_ptr;
  char * filename;
  int pipe1[2]; /* written by host process, read by plugin UI process */
  int pipe2[2]; /* written by plugin UI process, read by host process */
  char ui_recv_pipe[100];
  char ui_send_pipe[100];
  int oldflags;
  FORK_TIME_MEASURE_VAR;
  const char * argv[6];
  int ret;
  int i;
  char ch;

  if (access("/usr/bin/python2", F_OK) != -1)
    argv[0] = "/usr/bin/python2";
  else if (access("/usr/bin/python2.7", F_OK) != -1)
    argv[0] = "/usr/bin/python2.7";
  else if (access("/usr/bin/python2.6", F_OK) != -1)
    argv[0] = "/usr/bin/python2.6";
  else
    goto fail;

  control_ptr = malloc(sizeof(struct control));
  if (control_ptr == NULL)
  {
    goto fail;
  }

  control_ptr->host = host;

  if (pipe(pipe1) != 0)
  {
    fprintf(stderr, "pipe1 creation failed.\n");
  }

  if (pipe(pipe2) != 0)
  {
    fprintf(stderr, "pipe2 creation failed.\n");
  }

  snprintf(ui_recv_pipe, sizeof(ui_recv_pipe), "%d", pipe1[0]); /* [0] means reading end */
  snprintf(ui_send_pipe, sizeof(ui_send_pipe), "%d", pipe2[1]); /* [1] means writting end */

  const char* bundle_path = "./resources/";

  filename = malloc(strlen(bundle_path) + strlen(UI_EXECUTABLE) + 1);
  if (filename == NULL)
  {
    goto fail_free_control;
  }

  strcpy(filename, bundle_path);
  strcat(filename, UI_EXECUTABLE);

  char sample_rate_str[12] = { 0 };
  snprintf(sample_rate_str, 12, "%g", host->get_sample_rate(host->handle));

  control_ptr->running = false;
  control_ptr->visible = false;

  control_ptr->pid = -1;

  argv[1] = filename;
  argv[2] = sample_rate_str;
  argv[3] = ui_recv_pipe;       /* reading end */
  argv[4] = ui_send_pipe;       /* writting end */
  argv[5] = NULL;

  FORK_TIME_MEASURE_BEGIN;

#if defined(USE_CLONE)
  {
    int stack[8000];

    ret = clone(clone_fn, stack + 4000, CLONE_VFORK, argv);
    if (ret == -1)
    {
      fprintf(stderr, "clone() failed: %s\n", strerror(errno));
      goto fail_free_control;
    }
  }
#elif defined(USE_CLONE2)
  fprintf(stderr, "clone2() exec not implemented yet\n");
  goto fail_free_control;
#else
  ret = FORK();
  switch (ret)
  {
  case 0:                       /* child process */
    /* fork duplicated the handles, close pipe ends that are used by parent process */
#if !defined(USE_VFORK)
    /* it looks we cant do this for vfork() */
    close(pipe1[1]);
    close(pipe2[0]);
#endif

    execvp(argv[0], (char **)argv);
    fprintf(stderr, "exec of UI failed: %s\n", strerror(errno));
    goto fail_free_control;
  case -1:
    fprintf(stderr, "fork() failed to create new process for plugin UI\n");
    goto fail_free_control;
  }

#endif

  FORK_TIME_MEASURE_END(FORK_STR "() time");

  //fprintf(stderr, FORK_STR "()-ed child process: %d\n", ret);
  control_ptr->pid = ret;

  /* fork duplicated the handles, close pipe ends that are used by the child process */
  close(pipe1[0]);
  close(pipe2[1]);

  control_ptr->send_pipe = pipe1[1]; /* [1] means writting end */
  control_ptr->recv_pipe = pipe2[0]; /* [0] means reading end */

  oldflags = fcntl(control_ptr->recv_pipe, F_GETFL);
  fcntl(control_ptr->recv_pipe, F_SETFL, oldflags | O_NONBLOCK);

  /* wait a while for child process to confirm it is alive */
  //printf("waiting UI start\n");
  i = 0;
loop:
  ret = read(control_ptr->recv_pipe, &ch, 1);
  switch (ret)
  {
  case -1:
    if (errno == EAGAIN)
    {
      if (i < WAIT_START_TIMEOUT / WAIT_STEP)
      {
        //printf("start wait %d ms ...\n", WAIT_STEP);
        usleep(WAIT_STEP * 1000);
        i++;
        goto loop;
      }

      fprintf(
        stderr,
        "we have waited for child with pid %d to appear for %.1f seconds and we are giving up\n",
        (int)control_ptr->pid,
        (float)((float)WAIT_START_TIMEOUT / 1000));
    }
    else
    {
      fprintf(stderr, "read() failed: %s\n", strerror(errno));
    }
    break;
  case 1:
    if (ch == '\n')
    {
      return control_ptr;
    }

    fprintf(stderr, "read() wrong first char '%c'\n", ch);

    break;
  default:
    fprintf(stderr, "read() returned %d\n", ret);
  }

  fprintf(stderr, "force killing misbehaved child %d (start)\n", (int)control_ptr->pid);

  if (kill(control_ptr->pid, SIGKILL) == -1)
  {
      fprintf(stderr, "kill() failed: %s (start)\n", strerror(errno));
  }

  /* wait a while child to exit, we dont like zombie processes */
  wait_child(control_ptr->pid);

fail_free_control:
  free(control_ptr);

fail:
  fprintf(stderr, "nekofilter UI launch failed\n");
  return NULL;
}

void
nekoui_cleanup(
  struct control * control_ptr)
{
  //printf("cleanup() called\n");
  free(control_ptr);
}

void nekoui_set_parameter_value(
  struct control * control_ptr,
  uint32_t index,
  float value)
{
  char buf[100];
  int len;
  char * locale;

  //printf("port_event(%u, %f) called\n", (unsigned int)port_index, *(float *)buffer);

  locale = strdup(setlocale(LC_NUMERIC, NULL));
  setlocale(LC_NUMERIC, "POSIX");

  write(control_ptr->send_pipe, "port_value\n", 11);
  len = sprintf(buf, "%u\n", (unsigned int)index);
  write(control_ptr->send_pipe, buf, len);
  len = sprintf(buf, "%.10f\n", value);
  write(control_ptr->send_pipe, buf, len);
  fsync(control_ptr->send_pipe);

  setlocale(LC_NUMERIC, locale);
  free(locale);
}

#undef control_ptr
