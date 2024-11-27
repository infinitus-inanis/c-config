#ifndef __FILE_MONITOR_H__
#define __FILE_MONITOR_H__

#include "utils/inotify-file.h"
#include <pthread.h>

typedef struct file_monitor file_monitor;

typedef struct {
  void (* on_create)(file_monitor *fm, void *udata);
  void (* on_remove)(file_monitor *fm, void *udata);
  void (* on_modify)(file_monitor *fm, void *udata);
} file_monitor_cbs;

file_monitor *
file_monitor_create(file_monitor_cbs cbs, const char *path, void *udata);

void
file_monitor_destroy(file_monitor *fm);

const char *
file_monitor_get_path(file_monitor *fm);

void
file_monitor_pause(file_monitor *fm);

void
file_monitor_resume(file_monitor *fm);

#endif//__FILE_MONITOR_H__
