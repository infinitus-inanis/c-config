#include "test-cfg.h"
#include "lib/xcfg-fld.h"

#include <unistd.h>
#include <stdlib.h>
#include <memory.h>

static xcfg_fld _type_obj[] = {
  XCFG_FLD1(struct type_obj, huh, XCFG_SFX_s08, upd, TYPE_OBJ_UPD_HUH),
};
static const xcfg_u32 _type_obj_sz = ARRAY_SIZE(_type_obj);

static xcfg_fld _type_cfg[] = {
  XCFG_FLD1(struct type_cfg, _s08, XCFG_SFX_s08, upd, TYPE_CFG_UPD_s08),
  XCFG_FLD1(struct type_cfg, _s16, XCFG_SFX_s16, upd, TYPE_CFG_UPD_s16),
  XCFG_FLD1(struct type_cfg, _s32, XCFG_SFX_s32, upd, TYPE_CFG_UPD_s32),
  XCFG_FLD1(struct type_cfg, _s64, XCFG_SFX_s64, upd, TYPE_CFG_UPD_s64),
  XCFG_FLD1(struct type_cfg, _u08, XCFG_SFX_u08, upd, TYPE_CFG_UPD_u08),
  XCFG_FLD1(struct type_cfg, _u16, XCFG_SFX_u16, upd, TYPE_CFG_UPD_u16),
  XCFG_FLD1(struct type_cfg, _u32, XCFG_SFX_u32, upd, TYPE_CFG_UPD_u32),
  XCFG_FLD1(struct type_cfg, _u64, XCFG_SFX_u64, upd, TYPE_CFG_UPD_u64),
  XCFG_FLD1(struct type_cfg, _f32, XCFG_SFX_f32, upd, TYPE_CFG_UPD_f32),
  XCFG_FLD1(struct type_cfg, _f64, XCFG_SFX_f64, upd, TYPE_CFG_UPD_f64),
  XCFG_FLD1(struct type_cfg, _ptr, XCFG_SFX_ptr, upd, TYPE_CFG_UPD_ptr),
  XCFG_FLD0(struct type_cfg, _obj, XCFG_SFX_obj, upd, TYPE_CFG_UPD_obj, _type_obj, _type_obj_sz),
  XCFG_FLD1(struct type_cfg, _str, XCFG_SFX_str, upd, TYPE_CFG_UPD_str),
};
static const xcfg_u32 _type_cfg_sz = ARRAY_SIZE(_type_cfg);

static xcfg_fld test_cfg_fld[] = {
  XCFG_FLD0(struct test_cfg, type,     XCFG_SFX_obj, upd, TEST_CFG_UPD_TYPE, _type_cfg, _type_cfg_sz),
  XCFG_FLD1(struct test_cfg, fake.val, XCFG_SFX_u08, upd, TEST_CFG_UPD_FAKE_VAL),
};

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
  return xcfg_create(DUMP_TYPE(struct test_cfg), DUMP_ARRAY(test_cfg_fld));
}

xcfg_ret
test_cfg_xbind(xcfg *ctx, struct test_cfg *cfg)
{
  return xcfg_set_data(ctx, cfg, sizeof *cfg);
}
