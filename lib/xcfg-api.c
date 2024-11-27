#define _GNU_SOURCE /* strdup, qsort, etc.*/

#include "xcfg-api.h"
#include "xcfg-tree.h"
#include "utils/file-monitor.h"

#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

#include <pthread.h>

#include <stdio.h>
#define logi(fmt, args...) printf("[xcfg]: "fmt "\n", ## args)

struct xcfg {
  xcfg_cbs   cbs;
  xcfg_rtti *rtti;
  xcfg_tree  tree;

  xcfg_u08        *data;
  pthread_mutex_t  data_mutex;

  struct {
    file_monitor *monitor;
  } file;
};

xcfg *
xcfg_create(xcfg_cbs cbs, xcfg_rtti *rtti)
{
  xcfg     *cfg;
  xcfg_ret  xret;
  int       ret;

  if (!rtti
   || !rtti->name[0]
   || !rtti->size
  ) return NULL;

  cfg = calloc(1, sizeof *cfg);
  if (!cfg)
    return NULL;

  cfg->cbs  = cbs;
  cfg->rtti = rtti;

  xret = xcfg_tree_build(&cfg->tree, cfg->rtti);
  if (xret < 0)
    goto error;

  if (cfg->cbs.on_create) {
    cfg->data = cfg->cbs.on_create();
  } else {
    cfg->data = calloc(cfg->rtti->size, sizeof *cfg->data);
  }

  if (!cfg->data)
    goto error;

  ret = pthread_mutex_init(&cfg->data_mutex, NULL);
  if (ret < 0)
    goto error;

  return cfg;

error:
  xcfg_destroy(cfg);
  return NULL;
}

void
xcfg_destroy(xcfg *cfg)
{
  if (!cfg)
    return;

  if (cfg->file.monitor) {
    file_monitor_destroy(cfg->file.monitor);
    cfg->file.monitor = NULL;
  }

  pthread_mutex_destroy(&cfg->data_mutex);

  if (cfg->cbs.on_destroy) {
    cfg->cbs.on_destroy(cfg->data);
  } else {
    free(cfg->data);
  }

  xcfg_tree_dispose(&cfg->tree);
  free(cfg);
}

void
xcfg_dump(xcfg *cfg)
{
  logi("data: %p", cfg->data);
  logi("type.name: %s", cfg->rtti->name);
  logi("type.size: %u", cfg->rtti->size);
  xcfg_tree_dump(&cfg->tree, cfg->data);
}

xcfg_ptr
xcfg_get_data(xcfg *cfg)
{
  return (xcfg_ptr)(cfg->data);
}

static xcfg_node *
get_node_by_key(xcfg *cfg, xcfg_key *key, xcfg_tid tid)
{
  xcfg_node *node = NULL;

  switch (key->type) {
    case XCFG_KEY_OFF: {
      node = xcfg_tree_get_node_by_off(&cfg->tree, key->off);
    } break;
    case XCFG_KEY_STR: {
      node = xcfg_tree_get_node_by_str(&cfg->tree, key->str);
    } break;
  }
  if (!node)
    return NULL;

  if (tid != XCFG_TID_ANY && node->rtfi->tid != tid)
    return NULL;

  return node;
}

static xcfg_ret
do_set(xcfg *cfg, xcfg_node *node, xcfg_ptr pval)
{
  xcfg_ret xret;

  pthread_mutex_lock(&cfg->data_mutex);
  xret = xcfg_node_set_value(node, cfg->data, pval);
  pthread_mutex_unlock(&cfg->data_mutex);

  return xret;
}

static xcfg_ret
do_get(xcfg *cfg, xcfg_node *node, xcfg_ptr pval)
{
  xcfg_ret xret;

  pthread_mutex_lock(&cfg->data_mutex);
  xret = xcfg_node_get_value(node, cfg->data, pval);
  pthread_mutex_unlock(&cfg->data_mutex);

  return xret;
}

static xcfg_ret
do_set_by_key(xcfg *cfg, xcfg_key *key, xcfg_tid tid, xcfg_ptr pval)
{
  xcfg_node *node;

  if (!cfg)
    return XCFG_RET_INVALID;
  
  node = get_node_by_key(cfg, key, tid);
  if (!node)
    return XCFG_RET_UNKNOWN;
  
  return do_set(cfg, node, pval);
}

static xcfg_ret
do_get_by_key(xcfg *cfg, xcfg_key *key, xcfg_tid tid, xcfg_ptr pval)
{
  xcfg_node *node;

  if (!cfg)
    return XCFG_RET_INVALID;
  
  node = get_node_by_key(cfg, key, tid);
  if (!node)
    return XCFG_RET_UNKNOWN;
  
  return do_get(cfg, node, pval);
}

#define XCFG_SFX_DO_EXPAND(sfx) \
  xcfg_ret \
  XCFG_SET(sfx)(xcfg *cfg, xcfg_key key, XCFG_TYPE(sfx) val) \
  { return do_set_by_key(cfg, &key, XCFG_TID(sfx), (xcfg_ptr)(&val)); }

  XCFG_SFX_EXPAND_all()
#undef XCFG_SFX_DO_EXPAND

#define XCFG_SFX_DO_EXPAND(sfx) \
  xcfg_ret \
  XCFG_GET(sfx)(xcfg *cfg, xcfg_key key, XCFG_TYPE(sfx) *pval) \
  { return do_get_by_key(cfg, &key, XCFG_TID(sfx), (xcfg_ptr)(pval)); }

  XCFG_SFX_EXPAND_val()
  XCFG_SFX_EXPAND_str()
  /* getter for `XCFG_SFX_ptr` is a spacial case */
  /* getter for `XCFG_SFX_obj` is a special case */
#undef XCFG_SFX_DO_EXPAND

xcfg_ret
xcfg_get_ptr(xcfg *cfg, xcfg_key key, xcfg_ptr pptr)
{ return do_get_by_key(cfg, &key, XCFG_TID_ptr, (xcfg_ptr)(pptr)); }

xcfg_ret
xcfg_get_obj(xcfg *cfg, xcfg_key key, xcfg_obj obj)
{ return do_get_by_key(cfg, &key, XCFG_TID_obj, (xcfg_ptr)(obj)); }


xcfg_ret
xcfg_update(xcfg *cfg, void *udata)
{
  if (!cfg->cbs.on_update)
    return XCFG_RET_SUCCESS;

  pthread_mutex_lock(&cfg->data_mutex);
  cfg->cbs.on_update(cfg->data, udata);
  pthread_mutex_unlock(&cfg->data_mutex);

  return XCFG_RET_SUCCESS;
}


xcfg_ret
xcfg_save_to_file(xcfg *cfg, const char *path)
{
  (void)cfg;
  logi("saving: %s", path);
  return XCFG_RET_SUCCESS;
}

xcfg_ret
xcfg_load_from_file(xcfg *cfg, const char *path)
{
  (void)cfg;
  logi("loading: %s", path);
  return XCFG_RET_SUCCESS;
}

static void
on_file_create(file_monitor *fm, void *udata)
{
  const char *path = file_monitor_get_path(fm);
  xcfg       *cfg  = udata;

  xcfg_load_from_file(cfg, path);
}

static void
on_file_modify(file_monitor *fm, void *udata)
{
  const char *path = file_monitor_get_path(fm);
  xcfg       *cfg  = udata;

  xcfg_load_from_file(cfg, path);
}

static file_monitor_cbs fm_cbs = {
  .on_create = on_file_create,
  .on_modify = on_file_modify,
};

xcfg_ret
xcfg_monitor_file(xcfg *cfg, const char *path)
{
  cfg->file.monitor = file_monitor_create(fm_cbs, path, cfg);
  if (!cfg->file.monitor)
    return XCFG_RET_FAILRUE;

  return XCFG_RET_SUCCESS;
}