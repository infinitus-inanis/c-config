#ifndef __XCFG_FILE_H__
#define __XCFG_FILE_H__

#include "xcfg-tree.h"
#include "utils/inotify-file.h"

#include <limits.h>
#include <pthread.h>

typedef struct xcfg_file xcfg_file;

typedef struct {
  xcfg_tree *tree;
  xcfg_ptr   data;
} xcfg_inotify_ctx;

typedef struct {
  pthread_t        tid;
  inotify_file     in;
  xcfg_inotify_ctx ctx;
  bool             alive;
} xcfg_inotify;

struct xcfg_file {
  xcfg_inotify monitor;
};

void
xcfg_file_dispose(xcfg_file *file);

xcfg_ret
xcfg_file_save(xcfg_file *file, xcfg_tree *tree, xcfg_ptr data);

xcfg_ret
xcfg_file_load(xcfg_file *file, xcfg_tree *tree, xcfg_ptr data);

xcfg_ret
xcfg_file_monitor(xcfg_file *file, xcfg_str path, xcfg_tree *tree, xcfg_ptr data);

#endif//__XCFG_FILE_H__
