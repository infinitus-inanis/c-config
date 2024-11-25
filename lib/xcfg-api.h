#ifndef __XCFG_API_H__
#define __XCFG_API_H__

#include "xcfg-types.h"

XCFG_EXPORT_ENTER

xcfg *
xcfg_create(xcfg_rtti *rtti, xcfg_on_update on_update, xcfg_on_dispose on_dispose);

void
xcfg_destroy(xcfg *ctx);

void
xcfg_dump(xcfg *ctx);

xcfg_ret
xcfg_set_data(xcfg *ctx, xcfg_ptr data, xcfg_u32 size);

xcfg_ptr
xcfg_get_data(xcfg *ctx);

xcfg_ret
xcfg_invoke_upd(xcfg *ctx, xcfg_ptr data);

xcfg_ret
xcfg_bind_file(xcfg *ctx, xcfg_str path);

xcfg_ret
xcfg_save_file(xcfg *ctx);

xcfg_ret
xcfg_load_file(xcfg *ctx);

xcfg_ret
xcfg_monitor_file(xcfg *ctx);

#define XCFG_SET_BY_OFF(sfx) CONCATENATE(xcfg_set_by_off_, sfx)
#define XCFG_SFX_DO_EXPAND(sfx) \
  xcfg_ret XCFG_SET_BY_OFF(sfx)(xcfg *ctx, xcfg_off off, XCFG_TYPE(sfx) val);

  XCFG_SFX_EXPAND_all()
#undef XCFG_SFX_DO_EXPAND

#define XCFG_GET_BY_OFF(sfx) CONCATENATE(xcfg_get_by_off_, sfx)
#define XCFG_SFX_DO_EXPAND(sfx) \
  xcfg_ret XCFG_GET_BY_OFF(sfx)(xcfg *ctx, xcfg_off off, XCFG_TYPE(sfx) *pval);

  XCFG_SFX_EXPAND_val()
  XCFG_SFX_EXPAND_str()
  /* getter for `XCFG_SFX_ptr` is a spacial case */
  /* getter for `XCFG_SFX_obj` is a special case */
#undef XCFG_SFX_DO_EXPAND

#define XCFG_SET_BY_KEY(sfx) CONCATENATE(xcfg_set_by_key_, sfx)
#define XCFG_SFX_DO_EXPAND(sfx) \
  xcfg_ret XCFG_SET_BY_KEY(sfx)(xcfg *ctx, xcfg_str key, XCFG_TYPE(sfx) val);

  XCFG_SFX_EXPAND_all()
#undef XCFG_SFX_DO_EXPAND

#define XCFG_GET_BY_KEY(sfx) CONCATENATE(xcfg_get_by_key_, sfx)
#define XCFG_SFX_DO_EXPAND(sfx) \
  xcfg_ret XCFG_GET_BY_KEY(sfx)(xcfg *ctx, xcfg_str key, XCFG_TYPE(sfx) *pval);

  XCFG_SFX_EXPAND_val()
  XCFG_SFX_EXPAND_str()
  /* getter for `XCFG_SFX_ptr` is a spacial case */
  /* getter for `XCFG_SFX_obj` is a special case */
#undef XCFG_SFX_DO_EXPAND

/*******************************************
 * Getters for `XCFG_SFX_ptr` are special: *
 *
 * We want an argument to be `xcfg_ptr` to avoid compiler warnings */

xcfg_ret
XCFG_GET_BY_OFF(XCFG_SFX_ptr)(xcfg *ctx, xcfg_off off, xcfg_ptr pptr);

xcfg_ret
XCFG_GET_BY_KEY(XCFG_SFX_ptr)(xcfg *ctx, xcfg_str key, xcfg_ptr pptr);

/******************************************
 * Getters for `XCFG_SFX_obj are special: *
 *
 * We want not a `pointer to pointer to object` but
 * a pointer to object itself to copy value directly */

xcfg_ret
XCFG_GET_BY_OFF(XCFG_SFX_obj)(xcfg *ctx, xcfg_off off, xcfg_obj obj);

xcfg_ret
XCFG_GET_BY_KEY(XCFG_SFX_obj)(xcfg *ctx, xcfg_str key, xcfg_obj obj);

XCFG_EXPORT_LEAVE

#endif//__XCFG_API_H__
