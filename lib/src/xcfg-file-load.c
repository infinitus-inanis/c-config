#define _GNU_SOURCE /* strdup, qsort, etc.*/

#include "xcfg-file.h"
#include "xcfg-impl.h"
#include "xcfg-data.h"

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <memory.h>
#include <limits.h>
#include <errno.h>

#include <regex.h>
#include <ctype.h>
#include <inttypes.h>

#define load_logi(path, line, fmt, args...) \
  logi("[loading:%s:%u] " fmt, (path), (xcfg_u32)(line), ## args)

typedef struct {
  const char *exp;
  regex_t     reg;
} load_reg;

typedef enum {
  LOAD_REG_sxx,
  LOAD_REG_uxx,
  LOAD_REG_fxx,
  LOAD_REG_str,
  LOAD_REG_COUNT,
} load_reg_type;

static load_reg lt_load_reg[LOAD_REG_COUNT] = {
  [LOAD_REG_sxx] = { .exp = "^[-+]?([0-9]+|0x[a-fA-F0-9]+)$" },
  [LOAD_REG_uxx] = { .exp = "^([0-9]+|0x[a-fA-F0-9]+)$"      },
  [LOAD_REG_fxx] = { .exp = "^[-+]?[0-9]+(\\.[0-9]+)?$"      },
  [LOAD_REG_str] = { .exp = "^\".*\"$"                       },
};

static bool
load_reg_match(load_reg_type type, const char *vstr)
{
  load_reg  *r = &lt_load_reg[type];
  regmatch_t rm;
  int        ret;
  char       err[256];

  ret = regexec(&r->reg, vstr, 1, &rm, 0);
  if (ret != REG_NOERROR) {
    regerror(ret, &r->reg, err, sizeof err);
    logi("regex[%u] failed with error: %s", type, err);
    return false;
  }

  return true;
}

static void __load_ctor() __attribute__((constructor));
static void __load_ctor()
{
  load_reg *r;
  xcfg_u32  i;

  for (i = 0; i < LOAD_REG_COUNT; ++i) {
    r = &lt_load_reg[i];
    if (regcomp(&r->reg, r->exp, REG_EXTENDED) < 0)
      logi("regex[%u]: failed to compile", i);
  }
}

static void __load_dtor() __attribute__((destructor));
static void __load_dtor()
{
  load_reg *r;
  xcfg_u32  i;

  for (i = 0; i < LOAD_REG_COUNT; ++i) {
    r = &lt_load_reg[i];
    regfree(&r->reg);
  }
}

typedef xcfg_ret (* loader)(xcfg_node *node, xcfg_ptr data, const char *sval);

#define LOADER(sfx) CONCATENATE(XCFG_TYPE(sfx), _load)

#define XCFG_SFX_DO_EXPAND(sfx, min, max) \
  static xcfg_ret \
  LOADER(sfx)(xcfg_node *node, xcfg_ptr data, const char *sval) \
  { \
    int   base = 10; \
    char *temp = (char *)(sval); \
    if (!load_reg_match(LOAD_REG_sxx, sval)) \
      return XCFG_RET_INVALID; \
    if (temp[0] == '-') \
      temp++; \
    if (temp[0] == '0' && (temp[1] == 'x' || temp[1] == 'X')) \
      base = 16; \
    intmax_t check = strtoimax(sval, &temp, base); \
    if (temp[0] != '\0') \
      return XCFG_RET_INVALID; \
    if (errno == EINVAL || errno == ERANGE) \
      return XCFG_RET_INVALID; \
    if (check < min || max < check) \
      return XCFG_RET_INVALID; \
    XCFG_TYPE(sfx) tval = (XCFG_TYPE(sfx))(check); \
    return xcfg_node_set_value(node, data, (xcfg_ptr)(&tval)); \
  }

  XCFG_SFX_EXPAND_s08(INT8_MIN,  INT8_MAX)
  XCFG_SFX_EXPAND_s16(INT16_MIN, INT16_MAX)
  XCFG_SFX_EXPAND_s32(INT32_MIN, INT32_MAX)
  XCFG_SFX_EXPAND_s64(INT64_MIN, INT64_MAX)
#undef XCFG_SFX_DO_EXPAND

#define XCFG_SFX_DO_EXPAND(sfx, max) \
  static xcfg_ret \
  LOADER(sfx)(xcfg_node *node, xcfg_ptr data, const char *sval) \
  { \
    int   base = 10; \
    char *temp = (char *)(sval); \
    if (!load_reg_match(LOAD_REG_sxx, sval)) \
      return XCFG_RET_INVALID; \
    if (temp[0] == '0' && (temp[1] == 'x' || temp[1] == 'X')) \
      base = 16; \
    uintmax_t check = strtoumax(sval, &temp, base); \
    if (temp[0] != '\0') \
      return XCFG_RET_INVALID; \
    if (errno == EINVAL || errno == ERANGE) \
      return XCFG_RET_INVALID; \
    if (check > max) \
      return XCFG_RET_INVALID; \
    XCFG_TYPE(sfx) tval = (XCFG_TYPE(sfx))(check); \
    return xcfg_node_set_value(node, data, (xcfg_ptr)(&tval)); \
  }

  XCFG_SFX_EXPAND_u08(UINT8_MAX)
  XCFG_SFX_EXPAND_u16(UINT16_MAX)
  XCFG_SFX_EXPAND_u32(UINT32_MAX)
  XCFG_SFX_EXPAND_u64(UINT64_MAX)
#undef XCFG_SFX_DO_EXPAND

static xcfg_ret
LOADER(XCFG_SFX_f32)(xcfg_node *node, xcfg_ptr data, const char *sval)
{
  char *temp;
  if (!load_reg_match(LOAD_REG_fxx, sval))
    return XCFG_RET_FAILRUE;

  float check = strtof(sval, &temp);
  if (temp[0] != '\0' || errno == ERANGE)
    return XCFG_RET_INVALID;

  XCFG_TYPE(XCFG_SFX_f32) tval = check;
  return xcfg_node_set_value(node, data, (xcfg_ptr)(&tval));
}

static xcfg_ret
LOADER(XCFG_SFX_f64)(xcfg_node *node, xcfg_ptr data, const char *sval)
{
  char *temp;
  if (!load_reg_match(LOAD_REG_fxx, sval))
    return XCFG_RET_FAILRUE;

  double check = strtod(sval, &temp);
  if (!temp || temp[0] != '\0' || errno == ERANGE)
    return XCFG_RET_INVALID;

  XCFG_TYPE(XCFG_SFX_f64) tval = check;
  return xcfg_node_set_value(node, data, (xcfg_ptr)(&tval));
}

static xcfg_ret
LOADER(XCFG_SFX_str)(xcfg_node *node, xcfg_ptr data, const char *sval)
{
  char *tval = (char *)(sval);

  if (!load_reg_match(LOAD_REG_str, sval))
    return XCFG_RET_FAILRUE;

  /* skip first & remove last quotes */
  tval++;
  *strrchr(tval, '"') = '\0';
  return xcfg_node_set_value(node, data, &tval);
}

static loader lt_loader[XCFG_TID_COUNT] = {
#define XCFG_SFX_DO_EXPAND(sfx) \
  [XCFG_TID(sfx)] = LOADER(sfx),

  XCFG_SFX_EXPAND_val()
  XCFG_SFX_EXPAND_str()
  /* `XCFG_SFX_ptr` is a runtime-only feature: no support for load */
  /* `XCFG_SFX_obj` is a special case that handled manualy */
#undef XCFG_SFX_DO_EXPAND
};

xcfg_ret
xcfg_load_file(xcfg *cfg, const char *path)
{
  FILE   *stream;
  char    rbuf[LINE_MAX];
  size_t  rlen = sizeof(rbuf) - 1;
  size_t  line = 0;
  char   *t0, *t1;

  xcfg_tree *tree = &cfg->tree;
  xcfg_ret   xret = XCFG_RET_FAILRUE;

  if (!path || !path[0])
    return XCFG_RET_INVALID;

  stream = fopen(path, "r");
  if (!stream)
    return XCFG_RET_INVALID;

  memset(rbuf, 0, sizeof rbuf);
  line = 0;

  fseek(stream, 0, SEEK_SET);
  { /* load header */
    xcfg_u32 type_size;

    if (line++, !fgets(rbuf, rlen, stream))
      goto out;

    t0 = rbuf;
    if (strncmp(t0, "0x", 2))
      goto out;
    t0 += 2;

    type_size = strtoul(t0, NULL, 16);
    if (type_size != tree->rtti->size) {
      load_logi(path, line, "type size is invalid: %u != %u", type_size, tree->rtti->size);
      goto out;
    }

    memset(rbuf, 0, sizeof rbuf);
    if (line++, !fgets(rbuf, rlen, stream))
      goto out;

    t0 = strchr(rbuf, '(');
    if (!t0) goto out;
    t1 = strchr(t0, ')');
    if (!t1) goto out;

    t0++;
    while (t0 < t1 && isspace(*t0)) ++t0;
    if (t0 == t1) goto out;
    t1--;
    while (t1 > t0 && isspace(*t1)) --t1;
    if (t0 == t1) goto out;
    *(t1 + 1) = '\0';
    /* here `t0` is a valid type name string */

    if (strcmp(t0, tree->rtti->name)) {
      load_logi(path, line, "type name is invalid: '%s' != '%s'", t0, tree->rtti->name);
      goto out;
    }
  }
  { /* load fields */
    xcfg_node *base = &tree->root;

    xcfg_lock_data(cfg);
    memset(rbuf, 0, sizeof rbuf);
    while (line++, fgets(rbuf, rlen, stream) && base) {
      xcfg_node *node;
      loader     nldr;
      xcfg_str   eq;
      xcfg_u32   i;

      t0 = rbuf;
      while (*t0 && isspace(*t0)) ++t0;

      /* skip commented lines */
      if (*t0 == '#') {
        load_logi(path, line, "commented");
        continue;
      }

      /* pop 'object stack' */
      if (*t0 == '}') {
        base = base->prev;
        continue;
      }

      /* skip non-field lines */
      if (*t0 != '.') continue;

      t0++;
      eq = t1 = strchr(t0, '=');
      if (!t1) continue;
      t1--;
      if (t1 < t0) continue;
      while (t1 > t0 && isspace(*t1)) --t1;
      *(t1 + 1) = '\0';
      /* `t0` is a valid `key` here */

      /* search for node in base */
      for (i = 0; i < base->nnext; ++i) {
        node = base->next[i];
        if (!strcmp(t0, node->rtfi->str))
          break;
      }
      if (i == base->nnext) {
        load_logi(path, line, "unknown key: %s", t0);
        continue;
      }

      /* push 'object stack' */
      if (node->rtfi->tid == XCFG_TID_obj) {
        base = node;
        continue;
      }

      t0 = eq;
      t0++;
      while (*t0 && isspace(*t0)) ++t0;

      t1 = strchr(t0, ',');
      if (!t1) continue;
      t1--;
      if (t1 < t0) continue;
      while (t1 > t0 && isspace(*t1)) --t1;
      *(t1 + 1) = '\0';
      /* `t0` is a valid `value` here */

      nldr = lt_loader[node->rtfi->tid];
      if (!nldr) {
        load_logi(path, line, "loader not found for: %s", node->rtfi->str);
        continue;
      }
      xret = nldr(node, xcfg_get_data(cfg), t0);
      if (xret < 0) {
        load_logi(path, line, "failed to load value for: %s", node->rtfi->str);
        continue;
      }
    }
    xcfg_notify_update_unlocked(cfg);
    xcfg_unlock_data(cfg);
  }
  xret = XCFG_RET_SUCCESS;

out:
  fclose(stream);
  return xret;
}