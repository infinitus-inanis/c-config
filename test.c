#include "test-cfg.h"

#include <stdio.h>
#include <memory.h>

#define logi(fmt, args...)          printf("[test]: " fmt "\n", ## args)
#define logii(depth, fmt, args...)  logi(INDENT(fmt), INDENTARG(depth), ## args)

static void
test_cfg_dump(char *pfx, struct test_cfg *cfg)
{
  logi("[%s]", pfx);
  logii(1, ".upd: 0x%08lx", cfg->upd);
  logii(1, ".fake.val: %u", cfg->fake.val);
  logii(1, ".type");
  logii(2, ".upd: 0x%08lx", cfg->type.upd);
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

static void
on_update(struct test_cfg *cfg, void *data)
{
  test_cfg_dump((char *)(data), cfg);
}

int main() {
  xcfg            *cfg;
  struct test_cfg *cfg_data, tmp_data;
  struct type_obj  obj_data;
  xcfg_ret         ret = XCFG_RET_SUCCESS;
  xcfg_str         cfg_path = "test.cfg";

  memset(&tmp_data, 0, sizeof tmp_data);
  memset(&obj_data, 0, sizeof obj_data);

  logi("init...");
  cfg = test_cfg_xcreate(on_update);
  if (!cfg) {
    logi("test_cfg_xcreate failure");
    return 1;
  }
  cfg_data = xcfg_get_data(cfg);

  if (1) { logi("test (by_off)");
    if (1) { logi("test (set_by_off)...");
      /* signed   */
      ret |= xcfg_set_by_off_s08(cfg, test_cfg_member(type._s08), -8);
      ret |= xcfg_set_by_off_s16(cfg, test_cfg_member(type._s16), -16);
      ret |= xcfg_set_by_off_s32(cfg, test_cfg_member(type._s32), -32);
      ret |= xcfg_set_by_off_s64(cfg, test_cfg_member(type._s64), -64);
      /* unsigned */
      ret |= xcfg_set_by_off_u08(cfg, test_cfg_member(type._u08), 8);
      ret |= xcfg_set_by_off_u16(cfg, test_cfg_member(type._u16), 16);
      ret |= xcfg_set_by_off_u32(cfg, test_cfg_member(type._u32), 32);
      ret |= xcfg_set_by_off_u64(cfg, test_cfg_member(type._u64), 64);
      /* float    */
      ret |= xcfg_set_by_off_f32(cfg, test_cfg_member(type._f32), 0.32f);
      ret |= xcfg_set_by_off_f64(cfg, test_cfg_member(type._f64), 0.64);

      obj_data.huh = 42;
      ret |= xcfg_set_by_off_ptr(cfg, test_cfg_member(type._ptr), &obj_data);
      ret |= xcfg_set_by_off_obj(cfg, test_cfg_member(type._obj), &obj_data);
      obj_data.huh = 69;

      ret |= xcfg_set_by_off_str(cfg, test_cfg_member(type._str), "hello world");

      if (ret != XCFG_RET_SUCCESS) {
        logi("...failure: %d", ret);
        goto error;
      }
      logi("...success");
      xcfg_invoke_upd(cfg, "cfg_data");

    } /* set_by_off */
    if (0) { logi("test (get_by_off)...");
      /* signed   */
      ret |= xcfg_get_by_off_s08(cfg, test_cfg_member(type._s08), &tmp_data.type._s08);
      ret |= xcfg_get_by_off_s16(cfg, test_cfg_member(type._s16), &tmp_data.type._s16);
      ret |= xcfg_get_by_off_s32(cfg, test_cfg_member(type._s32), &tmp_data.type._s32);
      ret |= xcfg_get_by_off_s64(cfg, test_cfg_member(type._s64), &tmp_data.type._s64);
      /* unsigned */
      ret |= xcfg_get_by_off_u08(cfg, test_cfg_member(type._u08), &tmp_data.type._u08);
      ret |= xcfg_get_by_off_u16(cfg, test_cfg_member(type._u16), &tmp_data.type._u16);
      ret |= xcfg_get_by_off_u32(cfg, test_cfg_member(type._u32), &tmp_data.type._u32);
      ret |= xcfg_get_by_off_u64(cfg, test_cfg_member(type._u64), &tmp_data.type._u64);
      /* float    */
      ret |= xcfg_get_by_off_f32(cfg, test_cfg_member(type._f32), &tmp_data.type._f32);
      ret |= xcfg_get_by_off_f64(cfg, test_cfg_member(type._f64), &tmp_data.type._f64);

      ret |= xcfg_get_by_off_ptr(cfg, test_cfg_member(type._ptr), &tmp_data.type._ptr);
      ret |= xcfg_get_by_off_obj(cfg, test_cfg_member(type._obj), &tmp_data.type._obj);
      ret |= xcfg_get_by_off_str(cfg, test_cfg_member(type._str), &tmp_data.type._str);

      if (ret != XCFG_RET_SUCCESS) {
        logi("...failure: %d", ret);
        goto error;
      }
      logi("...success");
      test_cfg_dump("tmp_data", &tmp_data);
      test_cfg_dispose(&tmp_data);
    } /* get_by_off */
  } /* by_off */

  if (0) { logi("test (by_key)...");
    if (0) { logi("test (set_by_key)...");
      /* signed   */
      ret |= xcfg_set_by_key_s08(cfg, "type._s08", INT8_MIN);
      ret |= xcfg_set_by_key_s16(cfg, "type._s16", INT16_MIN);
      ret |= xcfg_set_by_key_s32(cfg, "type._s32", INT32_MIN);
      ret |= xcfg_set_by_key_s64(cfg, "type._s64", INT64_MIN);
      /* unsigned */
      ret |= xcfg_set_by_key_u08(cfg, "type._u08", UINT8_MAX);
      ret |= xcfg_set_by_key_u16(cfg, "type._u16", UINT16_MAX);
      ret |= xcfg_set_by_key_u32(cfg, "type._u32", UINT32_MAX);
      ret |= xcfg_set_by_key_u64(cfg, "type._u64", UINT64_MAX);
      /* float    */
      ret |= xcfg_set_by_key_f32(cfg, "type._f32", -0.32);
      ret |= xcfg_set_by_key_f64(cfg, "type._f64", -0.64);

      obj_data.huh = -42;
      ret |= xcfg_set_by_key_ptr(cfg, "type._ptr", &obj_data);
      ret |= xcfg_set_by_key_obj(cfg, "type._obj", &obj_data);
      obj_data.huh = -69;

      ret |= xcfg_set_by_key_str(cfg, "type._str", "dlrow olleh");

      if (ret != XCFG_RET_SUCCESS) {
        logi("...failure: %d", ret);
        goto error;
      }
      logi("...success");
      xcfg_invoke_upd(cfg, "cfg_data");
    } /* set_by_key */
    if (0) { logi("test (get_by_key)...");
      /* signed   */
      ret |= xcfg_get_by_key_s08(cfg, "type._s08", &tmp_data.type._s08);
      ret |= xcfg_get_by_key_s16(cfg, "type._s16", &tmp_data.type._s16);
      ret |= xcfg_get_by_key_s32(cfg, "type._s32", &tmp_data.type._s32);
      ret |= xcfg_get_by_key_s64(cfg, "type._s64", &tmp_data.type._s64);
      /* unsigned */
      ret |= xcfg_get_by_key_u08(cfg, "type._u08", &tmp_data.type._u08);
      ret |= xcfg_get_by_key_u16(cfg, "type._u16", &tmp_data.type._u16);
      ret |= xcfg_get_by_key_u32(cfg, "type._u32", &tmp_data.type._u32);
      ret |= xcfg_get_by_key_u64(cfg, "type._u64", &tmp_data.type._u64);
      /* float    */
      ret |= xcfg_get_by_key_f32(cfg, "type._f32", &tmp_data.type._f32);
      ret |= xcfg_get_by_key_f64(cfg, "type._f64", &tmp_data.type._f64);

      ret |= xcfg_get_by_key_ptr(cfg, "type._ptr", &tmp_data.type._ptr);
      ret |= xcfg_get_by_key_obj(cfg, "type._obj", &tmp_data.type._obj);

      ret |= xcfg_get_by_key_str(cfg, "type._str", &tmp_data.type._str);

      if (ret != XCFG_RET_SUCCESS) {
        logi("...failure: %d", ret);
        goto error;
      }
      logi("...success");
      test_cfg_dump("tmp_data", &tmp_data);
      test_cfg_dispose(&tmp_data);
    } /* set_by_key */
  } /* by_key */

  if (1) { logi("test (file)");
    if (1) { logi("test (bind_file)...");
      ret |= xcfg_bind_file(cfg, cfg_path);

      if (ret != XCFG_RET_SUCCESS) {
        logi("failure: %d", ret);
        goto error;
      }
      logi("...success");
    } /* bind_file */
    if (0) { logi("test (save_file)...");
      ret |= xcfg_save_file(cfg);

      if (ret != XCFG_RET_SUCCESS) {
        logi("failure: %d", ret);
        goto error;
      }
      logi("...success");
    } /* save_file */
    if (0) { logi("test (load_file)...");
      ret |= xcfg_load_file(cfg);

      if (ret != XCFG_RET_SUCCESS) {
        logi("failure: %d", ret);
        goto error;
      }
      logi("...success");
      test_cfg_dump("cfg_data", cfg_data);
    } /* load_file */
    if (1) { logi("test (monitor_file)...");
      ret |= xcfg_monitor_file(cfg);

      if (ret != XCFG_RET_SUCCESS) {
        logi("failure: %d", ret);
        goto error;
      }
      logi("...success");
    } /* monitor_file */
  } /* file */
error:
  test_cfg_dispose(&tmp_data);
  xcfg_destroy(cfg);

  return 0;
}