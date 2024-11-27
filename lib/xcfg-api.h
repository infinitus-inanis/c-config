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

xcfg_ptr
xcfg_get_data(xcfg *cfg);

typedef enum {
  XCFG_KEY_OFF,
  XCFG_KEY_STR,
} xcfg_key_type;

typedef struct {
  xcfg_key_type type;
union {
  xcfg_off off;
  xcfg_str str;
};
} xcfg_key;

#define xcfg_key_off(type, member) (xcfg_key) { XCFG_KEY_OFF, .off = FIELD_OFFSET_OF(type, member) }
#define xcfg_key_str(value)        (xcfg_key) { XCFG_KEY_STR, .str = (value) }

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
xcfg_get_ptr(xcfg *cfg, xcfg_key key, xcfg_ptr pptr);

/****************************
 * Getter for `XCFG_SFX_obj *
 *
 * We want not a `pointer to pointer to object` but
 * a `pointer to object` itself to copy value directly */
xcfg_ret
xcfg_get_obj(xcfg *cfg, xcfg_key key, xcfg_obj obj);


xcfg_ret
xcfg_update(xcfg *cfg, void *udata);


xcfg_ret
xcfg_save_to_file(xcfg *cfg, const char *path);

xcfg_ret
xcfg_load_from_file(xcfg *cfg, const char *path);

xcfg_ret
xcfg_monitor_file(xcfg *cfg, const char *path);

XCFG_EXPORT_LEAVE

#endif//__XCFG_API_H__
