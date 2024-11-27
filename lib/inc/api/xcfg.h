#ifndef __XCFG_API_H__
#define __XCFG_API_H__

#include "xcfg-types.h"

XCFG_EXPORT_ENTER

xcfg *
xcfg_create(xcfg_cbs cbs, xcfg_rtti *rtti);

void
xcfg_destroy(xcfg *cfg);

void
xcfg_dump(xcfg *cfg);

XCFG_EXPORT_LEAVE

#endif//__XCFG_API_H__
