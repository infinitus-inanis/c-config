#include "sig-thread.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

struct sig_thread {
  pthread_t     id;
  sigset_t      mask;
  sig_thread_f  call;
  void         *data;
};

static void * sig_thread_routine(void *arg) {
  struct sig_thread *thread = arg;
  siginfo_t info;

  memset(&info, 0, sizeof info);
  while (1) {
    sigwaitinfo(&thread->mask, &info);

    if (thread->call)
      if (thread->call(&info, thread->data) < 0)
        continue;

    break;
  }
  pthread_exit(NULL);
}

struct sig_thread *
sig_thread_create(const struct sig_thread_args *args) {
  struct sig_thread *thread;

  assert(args);
  assert(args->call);

  if (!(thread = calloc(1, sizeof *thread)))
    return NULL;

  memcpy(&thread->mask, &args->mask, sizeof thread->mask);

  thread->call = args->call;
  thread->data = args->data;

  if (pthread_sigmask(SIG_BLOCK, &thread->mask, NULL) < 0)
    goto error;

  if (pthread_create(&thread->id, NULL, sig_thread_routine, thread) < 0)
    goto error;

  if (pthread_detach(thread->id) < 0)
    goto error;

  return thread;

error:
  sig_thread_destroy(thread);

  return NULL;
}

void
sig_thread_destroy(struct sig_thread *thread) {
  if (!thread)
    return;

  free(thread);
}