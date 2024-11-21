#include "test-cfg.h"

#include <stdio.h>
#include <memory.h>

#define logi(fmt, args...) printf("[tst]: "fmt "\n", ## args)

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
  }

  if (0) {
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