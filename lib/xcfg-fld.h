#ifndef __XCFG_FLD_H__
#define __XCFG_FLD_H__

#include "xcfg-api-types.h"

/* Resolves type suffix to type field type id. */
#define XCFG_FLD_TYPE(sfx) CONCATENATE(XCFG_FLD_TYPE_, sfx)

#undef XCFG_SFX_DO_EXPAND
#define XCFG_SFX_DO_EXPAND(sfx) XCFG_FLD_TYPE(sfx),

typedef enum {
  XCFG_FLD_TYPE(INVAL),
  XCFG_SFX_EXPAND_ALL()
  XCFG_FLD_TYPE(COUNT),
} xcfg_fld_type;

#undef XCFG_SFX_DO_EXPAND

typedef struct {
  xcfg_u32 off;
  xcfg_u32 size;
} xcfg_ref_desc;

#define XCFG_REF_DESC(type, member) \
  { \
    .off  = FIELD_OFFSET_OF(type, member), \
    .size = FIELD_SIZE_OF(type, member) \
  }

typedef struct {
  xcfg_u08      id;
  xcfg_ref_desc ref;
} xcfg_upd_desc;

#define XCFG_UPD_DESC(upd_id, type, member) \
  { \
    .id = (upd_id), \
    .ref = XCFG_REF_DESC(type, member), \
  }

typedef struct xcfg_fld xcfg_fld;
struct xcfg_fld {
  xcfg_str       key;
  xcfg_ref_desc  ref;
  xcfg_fld_type  type;

  xcfg_upd_desc  upd;

  xcfg_fld *sub;
  xcfg_u32  nsub;
};

#define XCFG_FLD0(cfg_type, fld_path, xcfg_sfx, upd_path, upd_id, cfg_sub, cfg_nsub) \
  { \
    .key = #fld_path, \
    .type = XCFG_FLD_TYPE(xcfg_sfx), \
    .ref = XCFG_REF_DESC(cfg_type, fld_path), \
    .upd = XCFG_UPD_DESC(upd_id, cfg_type, upd_path), \
    .sub = (cfg_sub), \
    .nsub = (cfg_nsub), \
  }

#define XCFG_FLD1(cfg_type, fld_path, xcfg_sfx, upd_path, upd_id) \
  XCFG_FLD0(cfg_type, fld_path, xcfg_sfx, upd_path, upd_id, NULL, 0)

#endif//__XCFG_FLD_H__
