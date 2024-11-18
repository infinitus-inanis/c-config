#include "test-cfg.h"

#include <stdlib.h>

#define ARRAY_SIZE(array)	(sizeof(array) / sizeof((array)[0]))

static cfg_info type_obj_infos[] = {
  CFG_INFO_1(struct type_obj, huh, CFG_TID_s08, upd, TYPE_OBJ_UPD_HUH),
};

static cfg_info type_cfg_infos[] = {
  CFG_INFO_1(struct type_cfg, _s08, CFG_TID_s08, upd, TYPE_CFG_UPD_s08),
  CFG_INFO_1(struct type_cfg, _s16, CFG_TID_s16, upd, TYPE_CFG_UPD_s16),
  CFG_INFO_1(struct type_cfg, _s32, CFG_TID_s32, upd, TYPE_CFG_UPD_s32),
  CFG_INFO_1(struct type_cfg, _s64, CFG_TID_s64, upd, TYPE_CFG_UPD_s64),
  CFG_INFO_1(struct type_cfg, _u08, CFG_TID_u08, upd, TYPE_CFG_UPD_u08),
  CFG_INFO_1(struct type_cfg, _u16, CFG_TID_u16, upd, TYPE_CFG_UPD_u16),
  CFG_INFO_1(struct type_cfg, _u32, CFG_TID_u32, upd, TYPE_CFG_UPD_u32),
  CFG_INFO_1(struct type_cfg, _u64, CFG_TID_u64, upd, TYPE_CFG_UPD_u64),
  CFG_INFO_1(struct type_cfg, _f32, CFG_TID_f32, upd, TYPE_CFG_UPD_f32),
  CFG_INFO_1(struct type_cfg, _f64, CFG_TID_f64, upd, TYPE_CFG_UPD_f64),
  CFG_INFO_1(struct type_cfg, _ptr, CFG_TID_ptr, upd, TYPE_CFG_UPD_ptr),
  CFG_INFO_0(struct type_cfg, _obj, CFG_TID_obj, upd, TYPE_CFG_UPD_obj,
    type_obj_infos, ARRAY_SIZE(type_obj_infos)),
  CFG_INFO_1(struct type_cfg, _str, CFG_TID_str, upd, TYPE_CFG_UPD_str),
};

static cfg_info test_cfg_infos[] = {
  CFG_INFO_0(struct test_cfg, type, CFG_TID_obj, upd, TEST_CFG_UPD_TYPE,
    type_cfg_infos, ARRAY_SIZE(type_cfg_infos)),
  CFG_INFO_1(struct test_cfg, fake.val, CFG_TID_u08, upd, TEST_CFG_UPD_FAKE_VAL),
};

cfg_ctx *
test_cfg_ctx_create(struct test_cfg *cfg)
{
  cfg_u32 infos_total_n =
    ARRAY_SIZE(test_cfg_infos) +
    ARRAY_SIZE(type_cfg_infos) +
    ARRAY_SIZE(type_obj_infos);

  return cfg_ctx_create(cfg, sizeof *cfg,
    test_cfg_infos, ARRAY_SIZE(test_cfg_infos),
    0);
}

void
test_cfg_ctx_destroy(cfg_ctx *ctx)
{
  cfg_ctx_destroy(ctx);
}