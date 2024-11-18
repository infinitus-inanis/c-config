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
  logi("    ._str (%p): %s",      &cfg->type._str,      cfg->type._str);
}

int main() {
  struct test_cfg  cfg;
  struct test_cfg  tmp;
  struct type_obj  obj;
  cfg_ctx         *ctx;
  cfg_ret          ret = CFG_RET_SUCCESS;

  memset(&cfg, 0, sizeof cfg);
  memset(&tmp, 0, sizeof tmp);
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

  if (1) {
    logi("proc (set)...");
    ret |= cfg_ctx_set_by_ref_s08(ctx, &cfg.type._s08, -8);
    ret |= cfg_ctx_set_by_ref_s16(ctx, &cfg.type._s16, -16);
    ret |= cfg_ctx_set_by_ref_s32(ctx, &cfg.type._s32, -32);
    ret |= cfg_ctx_set_by_ref_s64(ctx, &cfg.type._s64, -64);

    ret |= cfg_ctx_set_by_ref_u08(ctx, &cfg.type._u08, 8);
    ret |= cfg_ctx_set_by_ref_u16(ctx, &cfg.type._u16, 16);
    ret |= cfg_ctx_set_by_ref_u32(ctx, &cfg.type._u32, 32);
    ret |= cfg_ctx_set_by_ref_u64(ctx, &cfg.type._u64, 64);

    ret |= cfg_ctx_set_by_ref_f32(ctx, &cfg.type._f32, 0.32f);
    ret |= cfg_ctx_set_by_ref_f64(ctx, &cfg.type._f64, 0.64);

    obj.huh = 42;
    ret |= cfg_ctx_set_by_ref_ptr(ctx, &cfg.type._ptr, &obj);
    ret |= cfg_ctx_set_by_ref_obj(ctx, &cfg.type._obj, &obj);
    obj.huh = 69;

    ret |= cfg_ctx_set_by_ref_str(ctx, &cfg.type._str, "hello world");

    if (ret != CFG_RET_SUCCESS) {
      logi("...failure: %d", ret);
      goto error;
    }
    logi("...success");
    logi("type_obj (%p)", &obj);
    logi("  .huh: %d", obj.huh);
    test_cfg_dump(&cfg);
  }

  if (0) {
    logi("proc (get)...");
    ret |= cfg_ctx_get_by_key_s08(ctx, "type._s08", &tmp.type._s08);
    ret |= cfg_ctx_get_by_key_s16(ctx, "type._s16", &tmp.type._s16);
    ret |= cfg_ctx_get_by_key_s32(ctx, "type._s32", &tmp.type._s32);
    ret |= cfg_ctx_get_by_key_s64(ctx, "type._s64", &tmp.type._s64);

    ret |= cfg_ctx_get_by_key_u08(ctx, "type._u08", &tmp.type._u08);
    ret |= cfg_ctx_get_by_key_u16(ctx, "type._u16", &tmp.type._u16);
    ret |= cfg_ctx_get_by_key_u32(ctx, "type._u32", &tmp.type._u32);
    ret |= cfg_ctx_get_by_key_u64(ctx, "type._u64", &tmp.type._u64);

    ret |= cfg_ctx_get_by_key_f32(ctx, "type._f32", &tmp.type._f32);
    ret |= cfg_ctx_get_by_key_f64(ctx, "type._f64", &tmp.type._f64);

    ret |= cfg_ctx_get_by_key_ptr(ctx, "type._ptr", (cfg_ptr *)(&tmp.type._ptr));
    ret |= cfg_ctx_get_by_key_obj(ctx, "type._obj", &tmp.type._obj);

    ret |= cfg_ctx_get_by_key_str(ctx, "type._str", &tmp.type._str);

    if (ret != CFG_RET_SUCCESS) {
      logi("...failure: %d", ret);
      goto error;
    }
    logi("...success");
    test_cfg_dump(&tmp);
  }

  if (1) {
    logi("proc (bind_file)...");
    ret |= cfg_ctx_bind_file(ctx, "test.cfg");

    if (ret != CFG_RET_SUCCESS) {
      logi("failure: %d", ret);
      goto error;
    }
    logi("...success");
  }

  if (1) {
    logi("proc (save_file)...");
    ret |= cfg_ctx_save_file(ctx);

    if (ret != CFG_RET_SUCCESS) {
      logi("failure: %d", ret);
      goto error;
    }
    logi("...success");
  }

//
//   logi("disp...");
//   memset(&cfg, 0, sizeof cfg);
//
//   logi("load...");
//   cfg_ctx_load_file(ctx);
//   test_cfg_dump(&cfg);

error:
  test_cfg_ctx_destroy(ctx);

  return 0;
}