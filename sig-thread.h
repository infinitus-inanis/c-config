#ifndef __SIG_THREAD_H__
#define __SIG_THREAD_H__

#include <sys/types.h>
#include <sys/signal.h>

struct sig_thread;

typedef int (*sig_thread_f)(siginfo_t *info, void *data);

struct sig_thread_args {
  sigset_t      mask;
  sig_thread_f  call;
  void         *data;
};

struct sig_thread *
sig_thread_create(const struct sig_thread_args *args);

void
sig_thread_destroy(struct sig_thread *thread);

#endif//__SIG_THREAD_H__
