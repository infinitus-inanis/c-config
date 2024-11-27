#ifndef __INOTIFY_FILE_H__
#define __INOTIFY_FILE_H__

#include <stdbool.h>

typedef struct {
  void (* on_create)(void *udata);
  void (* on_remove)(void *udata);
  void (* on_modify)(void *udata);
} inotify_file_cbs;

typedef struct {
  inotify_file_cbs cbs;

  int fd;

  struct {
    int   wd;
    char *path;
  } dir;

  struct {
    int   wd;
    char *path;
    char *name; /* points to position in `.path` where file name starts */

    struct {
      bool opened   : 1;
      bool modified : 1;
    } meta;
  } file;
} inotify_file;

int
inotify_file_setup(inotify_file     *in,
                   inotify_file_cbs  cbs,
                   const char       *path);

void
inotify_file_dispose(inotify_file *in);

int
inotify_file_execute(inotify_file *in, void *udata);


#endif//__INOTIFY_FILE_H__
