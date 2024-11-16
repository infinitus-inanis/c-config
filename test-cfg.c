#include "test-cfg.h"

#include <stdlib.h>

#define ARRAY_SIZE(array)	(sizeof(array) / sizeof((array)[0]))

#define CFG_ENT_TABLE(ent)                                                                        \
  ent(struct test_cfg, temp.mode,            CFG_TID_u32, temp.upd, TEMP_CFG_UPD_MODE)            \
  ent(struct test_cfg, temp.manual.strength, CFG_TID_u08, temp.upd, TEMP_CFG_UPD_MANUAL_STRENGTH)

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