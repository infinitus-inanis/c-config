#include "test-cfg.h"

#include <stdio.h>
#include <memory.h>

#define logi(fmt, args...) printf(fmt "\n", ## args)

static void
log_test_cfg(struct test_cfg const *cfg)
{
  logi("test_cfg(%p):", cfg);
  logi("  .temp");
  logi("    .upd: %#016lx", cfg->temp.upd);
  logi("    .mode: %u", cfg->temp.mode);
  logi("    .manual");
  logi("      .strength: %u", cfg->temp.manual.strength);
}

int main() {
  struct test_cfg  cfg;
  cfg_ctx_t       *ctx;

  memset(&cfg, 0, sizeof cfg);

  logi("init");
  log_test_cfg(&cfg);

  ctx = test_cfg_ctx_create(&cfg);
  if (!ctx) {
    logi("test_cfg_ctx_create failure");
    return 1;
  }

  logi("");
  logi("temp.mode set by key");
  cfg_ctx_set_by_key_u32(ctx, "temp.mode", TEMP_MODE_MANUAL);
  log_test_cfg(&cfg);

  logi("");
  logi("temp.manual.strength set by ref");
  cfg_ctx_set_by_ref_u08(ctx, &cfg.temp.manual.strength, 100);
  log_test_cfg(&cfg);

  test_cfg_ctx_destroy(ctx);
  
  return 0;
}