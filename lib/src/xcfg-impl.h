#ifndef __XCFG_IMPL_H__
#define __XCFG_IMPL_H__

#include "xcfg-tree.h"

#include <pthread.h>

typedef struct file_monitor file_monitor;

struct xcfg {
  xcfg_cbs   cbs;
  xcfg_rtti *rtti;
  xcfg_tree  tree;

  xcfg_ptr        data;
  pthread_mutex_t data_mutex;

  struct {
    file_monitor *monitor;
  } file;
};

#include <stdio.h>
#define logi(fmt, args...) printf("[xcfg]: "fmt "\n", ## args)

#endif//__XCFG_IMPL_H__
