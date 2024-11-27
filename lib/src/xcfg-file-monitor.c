#include "xcfg-file.h"
#include "xcfg-impl.h"
#include "utils/file-monitor.h"

static void
on_file_create(file_monitor *fm, void *udata)
{
  const char *path = file_monitor_get_path(fm);
  xcfg       *cfg  = udata;

  file_monitor_suspend(fm);
  xcfg_load_file(cfg, path);
  file_monitor_proceed(fm);
}

static void
on_file_modify(file_monitor *fm, void *udata)
{
  const char *path = file_monitor_get_path(fm);
  xcfg       *cfg  = udata;

  file_monitor_suspend(fm);
  xcfg_load_file(cfg, path);
  file_monitor_proceed(fm);
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
