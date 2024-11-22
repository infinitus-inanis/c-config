#ifndef __XCFG_FILE_H__
#define __XCFG_FILE_H__

#include "xcfg-tree.h"

#include <limits.h>
#include <pthread.h>

typedef struct xcfg_file xcfg_file;

typedef struct {
  int        fd;
  pthread_t  tid;
  bool       alive;

  xcfg_tree *tree;
  xcfg_ptr   data;

  struct {
    int      wd;
    xcfg_str path;
  } dir;

  struct {
    int      wd;
    xcfg_str name;
    xcfg_str path;
    bool     opened   : 1;
    bool     modified : 1;
  } file;
} xcfg_inotify;

struct xcfg_file {
  char dir [PATH_MAX - NAME_MAX - 1];
  char name[NAME_MAX - 1];
  char path[PATH_MAX];

  xcfg_inotify inotify;
};

void
xcfg_file_dispose(xcfg_file *file);

xcfg_ret
xcfg_file_set_path(xcfg_file *file, xcfg_str path);

xcfg_ret
xcfg_file_save(xcfg_file *file, xcfg_tree *tree, xcfg_ptr data);

xcfg_ret
xcfg_file_load(xcfg_file *file, xcfg_tree *tree, xcfg_ptr data);

xcfg_ret
xcfg_file_monitor(xcfg_file *file, xcfg_tree *tree, xcfg_ptr data);

#endif//__XCFG_FILE_H__
