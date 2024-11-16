#ifndef __CFG_TEST_H__
#define __CFG_TEST_H__

#include "cfg-base.h"

enum temp_mode {
  TEMP_MODE_INVALID = -1,
  TEMP_MODE_AUTO    =  0,
  TEMP_MODE_MANUAL  =  1,
};

enum temp_cfg_upd {
  TEMP_CFG_UPD_MODE            = CFG_UPD_FLAG(0),
  TEMP_CFG_UPD_MANUAL_STRENGTH = CFG_UPD_FLAG(1),

  TEMP_CFG_UPD_ALL = CFG_UPD_FLAG_ALL
};

struct temp_cfg {
  cfg_upd upd;

  enum temp_mode mode;

  struct {
    cfg_u08 strength;
  } manual;
};

struct test_cfg {
  struct temp_cfg temp;
};

cfg_ctx_t *
test_cfg_ctx_create(struct test_cfg const *cfg);

void
test_cfg_ctx_destroy(cfg_ctx_t *ctx);

#endif//__CFG_TEST_H__
