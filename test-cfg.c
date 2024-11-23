#include "test-cfg.h"
#include "lib/xcfg-rtti.h"

#include <unistd.h>
#include <stdlib.h>
#include <memory.h>

DECL_XCFG_RTTI(_type_obj,
  struct type_obj, upd,
  XCFG_RTFI_VAL(XCFG_TID_s08, struct type_obj, huh, TYPE_OBJ_UPD_HUH)
);

DECL_XCFG_RTTI(_type_cfg,
  struct type_cfg, upd,
  XCFG_RTFI_VAL(XCFG_TID_s08, struct type_cfg, _s08, TYPE_CFG_UPD_s08),
  XCFG_RTFI_VAL(XCFG_TID_s16, struct type_cfg, _s16, TYPE_CFG_UPD_s16),
  XCFG_RTFI_VAL(XCFG_TID_s32, struct type_cfg, _s32, TYPE_CFG_UPD_s32),
  XCFG_RTFI_VAL(XCFG_TID_s64, struct type_cfg, _s64, TYPE_CFG_UPD_s64),
  XCFG_RTFI_VAL(XCFG_TID_u08, struct type_cfg, _u08, TYPE_CFG_UPD_u08),
  XCFG_RTFI_VAL(XCFG_TID_u16, struct type_cfg, _u16, TYPE_CFG_UPD_u16),
  XCFG_RTFI_VAL(XCFG_TID_u32, struct type_cfg, _u32, TYPE_CFG_UPD_u32),
  XCFG_RTFI_VAL(XCFG_TID_u64, struct type_cfg, _u64, TYPE_CFG_UPD_u64),
  XCFG_RTFI_VAL(XCFG_TID_f32, struct type_cfg, _f32, TYPE_CFG_UPD_f32),
  XCFG_RTFI_VAL(XCFG_TID_f64, struct type_cfg, _f64, TYPE_CFG_UPD_f64),
  XCFG_RTFI_VAL(XCFG_TID_ptr, struct type_cfg, _ptr, TYPE_CFG_UPD_ptr),
  XCFG_RTFI_OBJ(&_type_obj,   struct type_cfg, _obj, TYPE_CFG_UPD_obj),
  XCFG_RTFI_VAL(XCFG_TID_str, struct type_cfg, _str, TYPE_CFG_UPD_str),
);

DECL_XCFG_RTTI(_test_cfg,
  struct test_cfg, upd,
  XCFG_RTFI_OBJ(&_type_cfg,   struct test_cfg, type,     TEST_CFG_UPD_TYPE),
  XCFG_RTFI_VAL(XCFG_TID_u08, struct test_cfg, fake.val, TEST_CFG_UPD_FAKE_VAL),
);

void
test_cfg_dispose(struct test_cfg *cfg)
{
  if (!cfg)
    return;

  free(cfg->type._str);
}

xcfg *
test_cfg_xcreate()
{
  return xcfg_create(&_test_cfg);
}

xcfg_ret
test_cfg_xbind(xcfg *ctx, struct test_cfg *cfg)
{
  return xcfg_set_data(ctx, cfg, sizeof *cfg);
}
