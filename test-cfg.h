#ifndef __CFG_TEST_H__
#define __CFG_TEST_H__

#include "lib/cfg-base.h"

enum type_obj_upd {
  TYPE_OBJ_UPD_HUH
};

struct type_obj {
  cfg_upd upd;

  cfg_s08 huh;
};

enum type_cfg_upd {
  TYPE_CFG_UPD_s08,
  TYPE_CFG_UPD_s16,
  TYPE_CFG_UPD_s32,
  TYPE_CFG_UPD_s64,
  TYPE_CFG_UPD_u08,
  TYPE_CFG_UPD_u16,
  TYPE_CFG_UPD_u32,
  TYPE_CFG_UPD_u64,
  TYPE_CFG_UPD_f32,
  TYPE_CFG_UPD_f64,
  TYPE_CFG_UPD_ptr,
  TYPE_CFG_UPD_obj,
  TYPE_CFG_UPD_str
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
  cfg_str          _str;
};

enum test_cfg_upd {
  TEST_CFG_UPD_TYPE,
  TEST_CFG_UPD_FAKE_VAL
};

struct test_cfg {
  cfg_upd upd;

  struct type_cfg type;
  struct { cfg_u08 val; } fake;
};

cfg_ctx *
test_cfg_ctx_create(struct test_cfg *cfg);

void
test_cfg_ctx_destroy(cfg_ctx *ctx);

#endif//__CFG_TEST_H__
