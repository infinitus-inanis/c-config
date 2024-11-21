#define _GNU_SOURCE /* strdup.*/

#include "xcfg-file.h"

#include <unistd.h>
#include <stdlib.h>
#include <memory.h>

xcfg_ret
xcfg_file_set_path(xcfg_file *file, xcfg_str path)
{
  strcpy(file->path, path);
  return XCFG_RET_SUCCESS;
}

xcfg_ret
xcfg_file_save(xcfg_file *file)
{
  (void)file;
  return XCFG_RET_SUCCESS;
}

xcfg_ret
xcfg_file_load(xcfg_file *file)
{
  (void)file;
  return XCFG_RET_SUCCESS;
}