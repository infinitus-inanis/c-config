#ifndef __XCFG_RTTI_H__
#define __XCFG_RTTI_H__

#include "xcfg-types.h"

typedef struct {
  xcfg_u32 off;
  xcfg_u32 size;
} xcfg_fld;

#define XCFG_FLD(type, member) \
  { \
    .off  = FIELD_OFFSET_OF(type, member), \
    .size = FIELD_SIZE_OF(type, member) \
  }

struct xcfg_rtti {
  xcfg_str   name;
  xcfg_u32   size;
  xcfg_fld   upd;
  xcfg_rtfi *rtfi;
  xcfg_u32   nrtfi;
};

#define XCFG_RTTI(type, upd_fld, _rtfi) \
  { \
    .name  = STRINGIFY(type), \
    .size  = sizeof(type), \
    .upd   = XCFG_FLD(type, upd_fld), \
    .rtfi  = (_rtfi), \
    .nrtfi = ARRAY_SIZE(_rtfi), \
  }

struct xcfg_rtfi {
  xcfg_tid tid;
  xcfg_str key;
  xcfg_fld fld;
  xcfg_u08 upd;

  /* used only if `.tid == XCFG_TID_obj` */
  struct {
    xcfg_rtti *rtti;
  } obj;
};

#define XCFG_RTFI(_tid, base_type, path, upd_id, obj_rtti) \
 { \
  .tid = (_tid), \
  .key = #path, \
  .fld = XCFG_FLD(base_type, path), \
  .upd = (upd_id), \
  .obj.rtti = (obj_rtti), \
 }

#define XCFG_RTFI_VAL(_tid, base_type, path, upd_id) \
  XCFG_RTFI(_tid, base_type, path, upd_id, NULL)

#define XCFG_RTFI_OBJ(obj_rtti, base_type, path, upd_id) \
  XCFG_RTFI(XCFG_TID_obj, base_type, path, upd_id, obj_rtti)

#define DECL_XCFG_RTTI(name, type, upd_path, flds...) \
  static xcfg_rtfi name##_rtfi[] = { flds }; \
  static xcfg_rtti name = XCFG_RTTI(type, upd_path, name##_rtfi)

#endif//__XCFG_RTTI_H__
