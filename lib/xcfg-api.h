#ifndef __XCFG_API_H__
#define __XCFG_API_H__

#include "xcfg-api-types.h"

xcfg *
xcfg_create(xcfg_str  type_name,
            xcfg_u32  type_size,
            xcfg_fld *root_pfld,
            xcfg_u32  root_nfld);

void
xcfg_destroy(xcfg *ctx);

xcfg_ret
xcfg_set_data(xcfg *ctx, xcfg_ptr data, xcfg_u32 size);

xcfg_ptr
xcfg_get_data(xcfg *ctx);

xcfg_ret
xcfg_bind_file(xcfg *ctx, xcfg_str path);

xcfg_ret
xcfg_save_file(xcfg *ctx);

xcfg_ret
xcfg_load_file(xcfg *ctx);

#define XCFG_SET_BY_REF(sfx) CONCATENATE(xcfg_set_by_ref_, sfx)
#define XCFG_SFX_DO_EXPAND(sfx) \
  xcfg_ret XCFG_SET_BY_REF(sfx)(xcfg *ctx, xcfg_ptr ref, XCFG_TYPE(sfx) val);

XCFG_SFX_EXPAND_ALL()
#undef XCFG_SFX_DO_EXPAND


#define XCFG_SET_BY_KEY(sfx) CONCATENATE(xcfg_set_by_key_, sfx)
#define XCFG_SFX_DO_EXPAND(sfx) \
  xcfg_ret XCFG_SET_BY_KEY(sfx)(xcfg *ctx, xcfg_str key, XCFG_TYPE(sfx) val);

XCFG_SFX_EXPAND_ALL()
#undef XCFG_SFX_DO_EXPAND


#define XCFG_GET_BY_KEY(sfx) CONCATENATE(xcfg_get_by_key_, sfx)
#define XCFG_SFX_DO_EXPAND(sfx) \
  xcfg_ret XCFG_GET_BY_KEY(sfx)(xcfg *ctx, xcfg_str key, XCFG_TYPE(sfx) *pval);

XCFG_SFX_EXPAND_VAL()
XCFG_SFX_EXPAND_PTR()
XCFG_SFX_EXPAND_STR()
#undef XCFG_SFX_DO_EXPAND

/*  Getter for `XCFG_SFX_obj` is a special case:
    We want not a 'pointer to pointer to object'
    but a pointer to object itself to copy value directly. */
xcfg_ret
XCFG_GET_BY_KEY(XCFG_SFX_obj)(xcfg *ctx, xcfg_str key, xcfg_obj obj);

#endif//__XCFG_API_H__
