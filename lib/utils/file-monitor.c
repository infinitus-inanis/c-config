#include "file-monitor.h"

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

struct file_monitor {
  file_monitor_cbs  cbs;
  inotify_file      in;
  void             *udata;
  pthread_t         tid;
  bool              talive;
  volatile bool     tpaused;
};

static void
in_on_create(void *udata)
{
  file_monitor *fm = udata;
  
  if (fm->cbs.on_create)
    fm->cbs.on_create(fm, fm->udata);
}

static void
in_on_remove(void *udata)
{
  file_monitor *fm = udata;
  
  if (fm->cbs.on_remove)
    fm->cbs.on_remove(fm, fm->udata);
}

static void
in_on_modify(void *udata)
{
  file_monitor *fm = udata;

  if (fm->cbs.on_modify)
    fm->cbs.on_modify(fm, fm->udata);
}

static inotify_file_cbs in_cbs = {
  .on_create = in_on_create,
  .on_remove = in_on_remove,
  .on_modify = in_on_modify,
};

static void *
file_monitor_routine(void *arg)
{
  file_monitor *fm = arg;
  int ret;

  while (fm->talive) {
    if (!fm->tpaused) {
      ret = inotify_file_execute(&fm->in, fm);
      if (ret < 0)
        break;
    }
    usleep(250 * 1000);
  }

  return NULL;
}

file_monitor *
file_monitor_create(file_monitor_cbs cbs, const char *path, void *udata)
{
  file_monitor *fm;
  int ret;

  fm = calloc(1, sizeof *fm);
  if (!fm)
    return NULL;

  fm->cbs = cbs;

  ret = inotify_file_setup(&fm->in, in_cbs, path);
  if (ret < 0)
    goto error;

  fm->udata = udata;

  fm->talive = true;
  ret = pthread_create(&fm->tid, NULL, file_monitor_routine, fm);
  if (ret < 0)
    goto error;

  return fm;

error:
  file_monitor_destroy(fm);
  return NULL;
}

void
file_monitor_destroy(file_monitor *fm)
{
  if (!fm)
    return;

  fm->talive = false;
  pthread_join(fm->tid, NULL);

  inotify_file_dispose(&fm->in);
  free(fm);
}

const char *
file_monitor_get_path(file_monitor *fm)
{
  assert(fm);

  return fm->in.file.path;
}

void
file_monitor_pause(file_monitor *fm)
{
  assert(fm);

  fm->tpaused = true;
}

void
file_monitor_resume(file_monitor *fm)
{
  assert(fm);

  fm->tpaused = false;
}
