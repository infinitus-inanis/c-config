#ifndef __XCFG_API_DATA_H__
#define __XCFG_API_DATA_H__

#include "xcfg-types.h"

xcfg_ptr
xcfg_get_data(xcfg *cfg);

void
xcfg_lock_data(xcfg *cfg);

void
xcfg_unlock_data(xcfg *cfg);

#define XCFG_SET(sfx) CONCATENATE(xcfg_set_, sfx)
#define XCFG_SFX_DO_EXPAND(sfx) \
  xcfg_ret XCFG_SET(sfx)(xcfg *cfg, xcfg_key key, XCFG_TYPE(sfx) val);

  XCFG_SFX_EXPAND_all()
#undef XCFG_SFX_DO_EXPAND

#define XCFG_GET(sfx) CONCATENATE(xcfg_get_, sfx)
#define XCFG_SFX_DO_EXPAND(sfx) \
  xcfg_ret XCFG_GET(sfx)(xcfg *cfg, xcfg_key key, XCFG_TYPE(sfx) *pval);

  XCFG_SFX_EXPAND_val()
  XCFG_SFX_EXPAND_str()
  /* getter for `XCFG_SFX_ptr` is a spacial case */
  /* getter for `XCFG_SFX_obj` is a special case */
#undef XCFG_SFX_DO_EXPAND

/****************************
 * Getter for `XCFG_SFX_ptr`*
 *
 * We want an argument to be `xcfg_ptr` to avoid compiler warnings
 * Real argument is `xcfg_ptr *` though */
xcfg_ret
XCFG_GET(XCFG_SFX_ptr)(xcfg *cfg, xcfg_key key, xcfg_ptr pptr);

/****************************
 * Getter for `XCFG_SFX_obj *
 *
 * We want not a `pointer to pointer to object` but
 * a `pointer to object` itself to copy value directly */
xcfg_ret
XCFG_GET(XCFG_SFX_obj)(xcfg *cfg, xcfg_key key, xcfg_obj obj);

#endif//__XCFG_API_DATA_H__
