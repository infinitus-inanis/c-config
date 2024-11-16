#ifndef __CFG_TEST_H__
#define __CFG_TEST_H__

#include "lib/cfg-base.h"

struct type_obj {
  cfg_s08 huh;
};

#define TYPE_CFG_UPD_NAME(tid) CONCATENATE(TYPE_CFG_UPD_, tid)

enum type_cfg_upd {
  EXPAND_CFG_TIDS(TYPE_CFG_UPD_NAME, EOL_COMMA)
};

struct type_cfg {
  cfg_upd  upd;

  cfg_s08 _s08;
  cfg_s16 _s16;
  cfg_s32 _s32;
  cfg_s64 _s64;

  cfg_u08 _u08;
  cfg_u16 _u16;
  cfg_u32 _u32;
  cfg_u64 _u64;

  cfg_f32 _f32;
  cfg_f64 _f64;

  struct type_obj *_ptr;
  struct type_obj  _obj;
};

struct test_cfg {
  struct type_cfg type;
};

cfg_ctx_t *
test_cfg_ctx_create(struct test_cfg const *cfg);

void
test_cfg_ctx_destroy(cfg_ctx_t *ctx);

#endif//__CFG_TEST_H__
