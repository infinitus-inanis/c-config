#include "test-cfg.h"

#include <stdlib.h>

#define ARRAY_SIZE(array)	(sizeof(array) / sizeof((array)[0]))

#define CFG_ENT_TABLE(ent)                                                 \
  ent(struct test_cfg, type._s08, CFG_TID_s08, type.upd, TYPE_CFG_UPD_s08) \
  ent(struct test_cfg, type._s16, CFG_TID_s16, type.upd, TYPE_CFG_UPD_s16) \
  ent(struct test_cfg, type._s32, CFG_TID_s32, type.upd, TYPE_CFG_UPD_s32) \
  ent(struct test_cfg, type._s64, CFG_TID_s64, type.upd, TYPE_CFG_UPD_s64) \
  ent(struct test_cfg, type._u08, CFG_TID_u08, type.upd, TYPE_CFG_UPD_u08) \
  ent(struct test_cfg, type._u16, CFG_TID_u16, type.upd, TYPE_CFG_UPD_u16) \
  ent(struct test_cfg, type._u32, CFG_TID_u32, type.upd, TYPE_CFG_UPD_u32) \
  ent(struct test_cfg, type._u64, CFG_TID_u64, type.upd, TYPE_CFG_UPD_u64) \
  ent(struct test_cfg, type._f32, CFG_TID_f32, type.upd, TYPE_CFG_UPD_f32) \
  ent(struct test_cfg, type._f64, CFG_TID_f64, type.upd, TYPE_CFG_UPD_f64) \
  ent(struct test_cfg, type._ptr, CFG_TID_ptr, type.upd, TYPE_CFG_UPD_ptr) \
  ent(struct test_cfg, type._obj, CFG_TID_obj, type.upd, TYPE_CFG_UPD_obj)

DECLARE_CFG_ENTS(test_cfg_ents, CFG_ENT_TABLE)

cfg_ctx_t *
test_cfg_ctx_create(struct test_cfg const *cfg)
{
  return cfg_ctx_create((void *)(cfg), test_cfg_ents, ARRAY_SIZE(test_cfg_ents));
}

void
test_cfg_ctx_destroy(cfg_ctx_t *ctx)
{
  cfg_ctx_destroy(ctx);
}