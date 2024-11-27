#ifndef __CFG_TEST_H__
#define __CFG_TEST_H__

#include "xcfg-api.h"

enum type_obj_upd {
  TYPE_OBJ_UPD_HUH
};

struct type_obj {
  xcfg_upd upd;

  xcfg_s08 huh;
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
  xcfg_upd  upd;

  xcfg_s08 _s08;
  xcfg_s16 _s16;
  xcfg_s32 _s32;
  xcfg_s64 _s64;
  xcfg_u08 _u08;
  xcfg_u16 _u16;
  xcfg_u32 _u32;
  xcfg_u64 _u64;
  xcfg_f32 _f32;
  xcfg_f64 _f64;

  struct type_obj *_ptr;
  struct type_obj  _obj;
  xcfg_str         _str;
};

enum test_cfg_upd {
  TEST_CFG_UPD_TYPE,
  TEST_CFG_UPD_FAKE_VAL
};

struct test_cfg {
  xcfg_upd upd;

  struct type_cfg type;
  struct { xcfg_u08 val; } fake;
};

#define test_cfg_key_off(member) xcfg_key_off(struct test_cfg, member)
#define test_cfg_key_str(value)  xcfg_key_str(value)

typedef void (* test_cfg_on_update)(struct test_cfg *cfg);

struct test_cfg *
test_cfg_create();

void
test_cfg_dispose(struct test_cfg *cfg);

void
test_cfg_destroy(struct test_cfg *cfg);

xcfg *
test_cfg_xcreate(test_cfg_on_update on_update);

#endif//__CFG_TEST_H__
