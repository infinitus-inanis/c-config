#include "xcfg-file.h"
#include "xcfg-impl.h"

xcfg_ret
xcfg_save_file(xcfg *cfg, const char *path)
{
  (void)cfg;
  logi("saving: %s", path);
  return XCFG_RET_SUCCESS;
}