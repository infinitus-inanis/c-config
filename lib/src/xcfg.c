#include "xcfg.h"
#include "xcfg-impl.h"
#include "utils/file-monitor.h"

#include <stdlib.h>

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
