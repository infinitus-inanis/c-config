#include "test-cfg.h"

#include <stdio.h>
#include <memory.h>

#define logi(fmt, args...) printf("[tst]: "fmt "\n", ## args)

static void
test_cfg_dump(struct test_cfg const *cfg)
{
  logi("test_cfg (%p):", cfg);
  logi("  .upd (%p): %#08lx",     &cfg->upd,            cfg->upd);
  logi("  .type (%p)",            &cfg->type);
  logi("    .upd (%p):  %#08lx",  &cfg->type.upd,       cfg->type.upd);
  logi("    ._s08 (%p): %d",      &cfg->type._s08,      cfg->type._s08);
  logi("    ._s16 (%p): %d",      &cfg->type._s16,      cfg->type._s16);
  logi("    ._s32 (%p): %d",      &cfg->type._s32,      cfg->type._s32);
  logi("    ._s64 (%p): %ld",     &cfg->type._s64,      cfg->type._s64);
  logi("    ._u08 (%p): %u",      &cfg->type._u08,      cfg->type._u08);
  logi("    ._u16 (%p): %u",      &cfg->type._u16,      cfg->type._u16);
  logi("    ._u32 (%p): %u",      &cfg->type._u32,      cfg->type._u32);
  logi("    ._u64 (%p): %lu",     &cfg->type._u64,      cfg->type._u64);
  logi("    ._f32 (%p): %f",      &cfg->type._f32,      cfg->type._f32);
  logi("    ._f64 (%p): %f",      &cfg->type._f64,      cfg->type._f64);
  logi("    ._obj (%p)",          &cfg->type._obj);
  logi("      .upd (%p): %#08lx", &cfg->type._obj.upd,  cfg->type._obj.upd);
  logi("      .huh (%p): %d",     &cfg->type._obj.huh,  cfg->type._obj.huh);
  logi("    ._ptr (%p): %p",      &cfg->type._ptr,      cfg->type._ptr);
  logi("      .upd (%p): %#08lx", &cfg->type._ptr->upd, cfg->type._ptr ? cfg->type._ptr->upd : 0);
  logi("      .huh (%p): %d",     &cfg->type._ptr->huh, cfg->type._ptr ? cfg->type._ptr->huh : 0);
}

int main() {
  struct test_cfg  cfg;
  struct test_cfg  tmp;
  struct type_obj  obj;
  cfg_ctx         *ctx;

  memset(&cfg, 0, sizeof cfg);
  memset(&cfg, 0, sizeof tmp);
  memset(&obj, 0, sizeof obj);

  logi("init...");
  logi("type_obj (%p)", &obj);
  logi("  .huh: %d", obj.huh);
  test_cfg_dump(&cfg);

  ctx = test_cfg_ctx_create(&cfg);
  if (!ctx) {
    logi("test_cfg_ctx_create failure");
    return 1;
  }

  logi("proc (set)...");

  cfg_ctx_set_by_ref_s08(ctx, &cfg.type._s08, -8);
  cfg_ctx_set_by_ref_s16(ctx, &cfg.type._s16, -16);
  cfg_ctx_set_by_ref_s32(ctx, &cfg.type._s32, -32);
  cfg_ctx_set_by_ref_s64(ctx, &cfg.type._s64, -64);

  cfg_ctx_set_by_ref_u08(ctx, &cfg.type._u08, 8);
  cfg_ctx_set_by_ref_u16(ctx, &cfg.type._u16, 16);
  cfg_ctx_set_by_ref_u32(ctx, &cfg.type._u32, 32);
  cfg_ctx_set_by_ref_u64(ctx, &cfg.type._u64, 64);

  cfg_ctx_set_by_ref_f32(ctx, &cfg.type._f32, 0.32f);
  cfg_ctx_set_by_ref_f64(ctx, &cfg.type._f64, 0.64);

  obj.huh = 42;

  cfg_ctx_set_by_ref_ptr(ctx, &cfg.type._ptr, &obj);
  cfg_ctx_set_by_ref_obj(ctx, &cfg.type._obj, &obj);

  obj.huh = 69;

  logi("done...");
  logi("type_obj (%p)", &obj);
  logi("  .huh: %d", obj.huh);
  test_cfg_dump(&cfg);

  cfg_ctx_bind_file(ctx, "test.cfg");

  logi("save...");
  cfg_ctx_save_file(ctx);

  logi("disp...");
  memset(&cfg, 0, sizeof cfg);

  logi("load...");
  cfg_ctx_load_file(ctx);
  test_cfg_dump(&cfg);

  test_cfg_ctx_destroy(ctx);

  return 0;
}