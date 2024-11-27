#ifndef __XCFG_API_FILE_H__
#define __XCFG_API_FILE_H__

#include "xcfg-types.h"

xcfg_ret
xcfg_save_file(xcfg *cfg, const char *path);

xcfg_ret
xcfg_load_file(xcfg *cfg, const char *path);

xcfg_ret
xcfg_monitor_file(xcfg *cfg, const char *path);

#endif//__XCFG_API_FILE_H__
