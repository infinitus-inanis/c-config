#ifndef __XCFG_FILE_H__
#define __XCFG_FILE_H__

#include "xcfg-tree.h"

#include <limits.h>
#include <pthread.h>

xcfg_ret
xcfg_file_save(xcfg_tree *tree, xcfg_ptr data);

xcfg_ret
xcfg_file_load(xcfg_tree *tree, xcfg_ptr data);

#endif//__XCFG_FILE_H__
