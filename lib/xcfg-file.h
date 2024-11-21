#ifndef __XCFG_FILE_H__
#define __XCFG_FILE_H__

#include "xcfg-types.h"

typedef struct {
  xcfg_str path;
} xcfg_file;

xcfg_ret
xcfg_file_set_path(xcfg_file *file, xcfg_str path);

xcfg_ret
xcfg_file_save(xcfg_file *file);

xcfg_ret
xcfg_file_load(xcfg_file *file);

#endif//__XCFG_FILE_H__
