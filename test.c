#include "test-cfg.h"

#include <stdio.h>
#include <memory.h>

#define logi(fmt, args...)   printf("[tst]: " fmt "\n", ## args)
#define logii(depth, fmt, args...)  logi(INDENT(fmt), INDENTARG(depth), ## args)

static void
test_cfg_dump(char *pfx, struct test_cfg *cfg)
{
  logi("[%s]", pfx);
  logii(1, ".upd: 0x%08lx", cfg->upd);
  logii(1, ".fake.val: %u", cfg->fake.val);
  logii(1, ".type");
  logii(2, ".upd: %lx", cfg->type.upd);
  logii(2, "._s08: %d", cfg->type._s08);
  logii(2, "._s16: %d", cfg->type._s16);
  logii(2, "._s32: %d", cfg->type._s32);
  logii(2, "._s64: %ld", cfg->type._s64);
  logii(2, "._u08: %u", cfg->type._u08);
  logii(2, "._u16: %u", cfg->type._u16);
  logii(2, "._u32: %u", cfg->type._u32);
  logii(2, "._u64: %lu", cfg->type._u64);
  logii(2, "._f32: %f", cfg->type._f32);
  logii(2, "._f64: %lf", cfg->type._f64);
  logii(2, "._ptr: %p", cfg->type._ptr);
  logii(3, ".upd: 0x%08lx", cfg->type._ptr ? cfg->type._ptr->upd : 0);
  logii(3, ".huh: %u", cfg->type._ptr ? cfg->type._ptr->huh : 0);
  logii(2, "._obj");
  logii(3, ".upd: 0x%08lx", cfg->type._obj.upd);
  logii(3, ".huh: %u", cfg->type._obj.huh);
  logii(2, "._str: %s", cfg->type._str);
}

int main() {
  struct test_cfg  cfg;
  struct test_cfg  tmp;
  struct type_obj  obj;
  xcfg            *ctx;
  xcfg_ret         ret = XCFG_RET_SUCCESS;

  memset(&cfg, 0, sizeof cfg);
  memset(&tmp, 0, sizeof tmp);
  memset(&obj, 0, sizeof obj);

  logi("init...");
  ctx = test_cfg_xcreate();
  if (!ctx) {
    logi("test_cfg_ctx_create failure");
    return 1;
  }
  test_cfg_xbind(ctx, &cfg);
  test_cfg_dump("cfg", &cfg);
  xcfg_dump(ctx);

  if (1) {
    logi("proc (set)...");
    ret |= xcfg_set_by_ref_s08(ctx, &cfg.type._s08, -8);
    ret |= xcfg_set_by_ref_s16(ctx, &cfg.type._s16, -16);
    ret |= xcfg_set_by_ref_s32(ctx, &cfg.type._s32, -32);
    ret |= xcfg_set_by_ref_s64(ctx, &cfg.type._s64, -64);

    ret |= xcfg_set_by_ref_u08(ctx, &cfg.type._u08, 8);
    ret |= xcfg_set_by_ref_u16(ctx, &cfg.type._u16, 16);
    ret |= xcfg_set_by_ref_u32(ctx, &cfg.type._u32, 32);
    ret |= xcfg_set_by_ref_u64(ctx, &cfg.type._u64, 64);

    ret |= xcfg_set_by_ref_f32(ctx, &cfg.type._f32, 0.32f);
    ret |= xcfg_set_by_ref_f64(ctx, &cfg.type._f64, 0.64);

    obj.huh = 42;
    ret |= xcfg_set_by_ref_ptr(ctx, &cfg.type._ptr, &obj);
    ret |= xcfg_set_by_ref_obj(ctx, &cfg.type._obj, &obj);
    obj.huh = 69;

    ret |= xcfg_set_by_ref_str(ctx, &cfg.type._str, "hello world");

    if (ret != XCFG_RET_SUCCESS) {
      logi("...failure: %d", ret);
      goto error;
    }
    logi("...success");
    test_cfg_dump("cfg", &cfg);
  }

  if (1) {
    logi("proc (get)...");
    ret |= xcfg_get_by_key_s08(ctx, "type._s08", &tmp.type._s08);
    ret |= xcfg_get_by_key_s16(ctx, "type._s16", &tmp.type._s16);
    ret |= xcfg_get_by_key_s32(ctx, "type._s32", &tmp.type._s32);
    ret |= xcfg_get_by_key_s64(ctx, "type._s64", &tmp.type._s64);

    ret |= xcfg_get_by_key_u08(ctx, "type._u08", &tmp.type._u08);
    ret |= xcfg_get_by_key_u16(ctx, "type._u16", &tmp.type._u16);
    ret |= xcfg_get_by_key_u32(ctx, "type._u32", &tmp.type._u32);
    ret |= xcfg_get_by_key_u64(ctx, "type._u64", &tmp.type._u64);

    ret |= xcfg_get_by_key_f32(ctx, "type._f32", &tmp.type._f32);
    ret |= xcfg_get_by_key_f64(ctx, "type._f64", &tmp.type._f64);

    ret |= xcfg_get_by_key_ptr(ctx, "type._ptr", (xcfg_ptr *)(&tmp.type._ptr));
    ret |= xcfg_get_by_key_obj(ctx, "type._obj", &tmp.type._obj);

    ret |= xcfg_get_by_key_str(ctx, "type._str", &tmp.type._str);

    if (ret != XCFG_RET_SUCCESS) {
      logi("...failure: %d", ret);
      goto error;
    }
    logi("...success");
    test_cfg_dump("cfg", &cfg);
  }

  if (0) {
    logi("proc (bind_file)...");
    ret |= xcfg_bind_file(ctx, "test.cfg");

    if (ret != XCFG_RET_SUCCESS) {
      logi("failure: %d", ret);
      goto error;
    }
    logi("...success");
  }

  if (0) {
    logi("proc (save_file)...");
    ret |= xcfg_save_file(ctx);

    if (ret != XCFG_RET_SUCCESS) {
      logi("failure: %d", ret);
      goto error;
    }
    logi("...success");
  }

  if (0) {
    logi("proc (load_file)...");
    memset(&tmp, 0, sizeof tmp);
    test_cfg_xbind(ctx, &tmp);

    ret |= xcfg_load_file(ctx);
    test_cfg_xbind(ctx, &cfg);

    if (ret != XCFG_RET_SUCCESS) {
      logi("failure: %d", ret);
      goto error;
    }
    logi("...success");
  }

error:
  xcfg_destroy(ctx);
  test_cfg_dispose(&tmp);
  test_cfg_dispose(&cfg);

  return 0;
}