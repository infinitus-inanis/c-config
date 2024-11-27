#define _GNU_SOURCE /* *_MAX macros */

#include "inotify-file.h"

#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>

#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/inotify.h>

#define IN_DIR_FLAGS \
  IN_DELETE_SELF              /* event  used to stop self                  */ \
  | IN_CREATE | IN_MOVED_TO   /* events used to start file-watch           */ \
  | IN_DELETE | IN_MOVED_FROM /* events used to stop  file-watch           */ \
  | IN_ONLYDIR                /* interested only in directory              */ \
  | IN_EXCL_UNLINK            /* not interested in children that moved out */

#define IN_FILE_FLAGS \
  IN_OPEN | IN_MODIFY | IN_CLOSE

#define IN_EVT_SIZE       (sizeof(struct inotify_event) + NAME_MAX + 1)
#define IN_EVTS_MAX       (8)
#define IN_EVTS_BUF_SIZE  (IN_EVT_SIZE * IN_EVTS_MAX)

static int
parse_file_path(const char *path, char **pdir, char **pname, char **pfull)
{
  char        pcopy[PATH_MAX];
  size_t      plen;
  char       *dir, *name, *full;
  char       *tstr, tc;
  ptrdiff_t   diff;

  plen = strnlen(path, PATH_MAX);
  if ((plen) > (PATH_MAX - 1))
    return -EINVAL;

  memcpy(pcopy, path, plen + 1);

  tc = '\0';
  tstr = strrchr(pcopy, '/');
  tstr = tstr ? tstr + 1 : pcopy;

  diff = tstr - pcopy;

  /* resolve directory first */
  if (diff == 0) {
    /* file name only */
    dir = get_current_dir_name();
  } else {
    tc = *tstr;   /* save name start char */
    *tstr = '\0'; /* separate directory   */
    /* `pcopy` is a valid directory here  */

    dir = realpath(pcopy, NULL);
  }

  if (!dir)
    return -ECANCELED;

  if (!dir[0]) {
    free(dir);
    return -ECANCELED;
  }

  if (tc)
    *tstr = tc; /* restore name start char */
  /* `tstr` is a valid name here           */

  plen = strnlen(tstr, NAME_MAX);
  if ((plen) > (NAME_MAX - 1)) {
    free(dir);
    return -ECANCELED;
  }

  plen = strnlen(dir, PATH_MAX);
  asprintf(&full, "%s%s%s", dir, (dir[plen - 1] == '/') ? "" : "/", tstr);

  plen = strnlen(full, PATH_MAX);
  if ((plen) > (PATH_MAX - 1)) {
    free(full);
    free(dir);
    return -ECANCELED;
  }

  name = strrchr(full, '/') + 1;

  *pdir  = dir;
  *pfull = full;
  *pname = name;
  return 0;
}

int
inotify_file_setup(inotify_file     *in,
                   inotify_file_cbs  cbs,
                   const char       *path)
{
  int ret;

  assert(in);
  assert(path && path[0]);

  memset(in, 0, sizeof *in);

  in->cbs = cbs;

  ret = parse_file_path(path, &in->dir.path, &in->file.name, &in->file.path);
  if (ret < 0)
    return ret;

  in->fd      = -1;
  in->dir.wd  = -1;
  in->file.wd = -1;

  in->fd = inotify_init1(O_NONBLOCK);
  if (in->fd < 0)
    goto error;

  { /* init dir watch */
    struct stat st;

    /* directory must exist */
    if (stat(in->dir.path, &st) || !(st.st_mode & S_IFDIR))
      goto error;

    in->dir.wd = inotify_add_watch(in->fd, in->dir.path, IN_DIR_FLAGS);
    if (in->dir.wd < 0)
      goto error;
  }
  { /* init file watch */
    struct stat st;

    /* create watch immediately if file already exist.
        only regular files are supported */
    if (!stat(in->file.path, &st) && (st.st_mode & S_IFREG)) {
      in->file.wd = inotify_add_watch(in->fd, in->file.path, IN_FILE_FLAGS);
      if (in->file.wd < 0)
        goto error;
    }
  }
  return 0;

error:
  inotify_file_dispose(in);
  return -1;
}

void
inotify_file_dispose(inotify_file *in)
{
  if (!in)
    return;

  close(in->fd);

  free(in->dir.path);
  free(in->file.path);
}

int
inotify_file_execute(inotify_file *in, void *udata)
{
  unsigned char rbuf[IN_EVTS_BUF_SIZE], *temp;
  ssize_t       rgot;

  struct inotify_event *e;

  rgot = read(in->fd, rbuf, sizeof rbuf);
  if (rgot < 0 && errno != EAGAIN)
    return -errno;

  if (rgot <= 0)
    return 0;

  for (temp  = rbuf;
       temp  < rbuf + rgot;
       temp += sizeof(*e) + e->len
  ) {
    e = (struct inotify_event *)(temp);
    if (e->wd == in->dir.wd) {
      if (e->mask & IN_DELETE_SELF) {
        in->dir.wd = -1;
        return -1;
      }
      if (e->mask & (IN_CREATE | IN_MOVED_TO)) {
        if (strcmp(e->name, in->file.name))
          continue;

        if (in->file.wd < 0)
          in->file.wd = inotify_add_watch(in->fd, in->file.path, IN_FILE_FLAGS);

        if (in->file.wd < 0)
          continue;

        if (in->cbs.on_create)
          in->cbs.on_create(udata);
        continue;
      }
      if (e->mask & (IN_DELETE | IN_MOVED_FROM)) {
        if (strcmp(e->name, in->file.name))
          continue;

        inotify_rm_watch(in->fd, in->file.wd);
        in->file.wd = -1;

        if (in->cbs.on_remove)
          in->cbs.on_remove(udata);
        continue;
      }
      continue;
    }

    if (e->wd == in->file.wd) {
      if (e->mask & IN_OPEN) {
        in->file.meta.opened = true;
        continue;
      }
      if (e->mask & IN_MODIFY) {
        in->file.meta.modified = true;

        if (!in->file.meta.opened)
          in->file.meta.modified = false;
        continue;
      }
      if (e->mask & IN_CLOSE) {
        if (in->file.meta.modified)
          if (in->cbs.on_modify)
            in->cbs.on_modify(udata);

        in->file.meta.modified = false;
        in->file.meta.opened   = false;
        continue;
      }
      continue;
    }
  }

  return 0;
}
