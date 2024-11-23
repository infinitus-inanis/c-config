#define _GNU_SOURCE /* strdup, qsort, etc.*/

#include "xcfg-file.h"
#include "utils/traverse.h"

#include <unistd.h>
#include <stdlib.h>
#include <memory.h>

#include <regex.h>
#include <ctype.h>
#include <inttypes.h>

#include <stdio.h>
#define logi(fmt, args...)         printf("[xcfg-file]: "fmt "\n", ## args)
#define logii(depth, fmt, args...) logi(INDENT(fmt), INDENTARG(depth), ## args)

typedef struct {
  bool (* save)(xcfg_ptr pfld, xcfg_str *pvstr);
  bool (* load)(xcfg_str vstr, xcfg_ptr pval);
} xcfg_type_ops;

typedef struct {
  FILE     *stream;
  xcfg_ptr  data;
} save_ctx;

#define XCFG_TYPE_DO_SAVE(sfx) CONCATENATE(XCFG_TYPE(sfx), _save)

#define XCFG_SFX_DO_EXPAND(sfx, fmt) \
  static bool \
  XCFG_TYPE_DO_SAVE(sfx)(xcfg_ptr pfld, xcfg_str *pvstr) \
  { \
    XCFG_TYPE(sfx) val = *XCFG_TYPE_CAST(sfx, pfld); \
    return asprintf(pvstr, fmt, val) >= 0; \
  }

  XCFG_SFX_EXPAND_s08("%"PRId8)
  XCFG_SFX_EXPAND_s16("%"PRId16)
  XCFG_SFX_EXPAND_s32("%"PRId32)
  XCFG_SFX_EXPAND_s64("%"PRId64)
  XCFG_SFX_EXPAND_u08("%"PRIu8)
  XCFG_SFX_EXPAND_u16("%"PRIu16)
  XCFG_SFX_EXPAND_u32("%"PRIu32)
  XCFG_SFX_EXPAND_u64("%"PRIu64)
  XCFG_SFX_EXPAND_f32("%f")
  XCFG_SFX_EXPAND_f64("%f")
  XCFG_SFX_EXPAND_str("\"%s\"")
#undef XCFG_SFX_DO_EXPAND

typedef struct {
  FILE     *stream;
  xcfg_ptr  data;
} load_ctx;

static xcfg_ptr load_tmp[XCFG_TID_COUNT] = {
#define XCFG_SFX_DO_EXPAND(sfx) \
  [XCFG_TID(sfx)] = XCFG_TYPE_TEMP(sfx),

  XCFG_SFX_EXPAND_val()
  XCFG_SFX_EXPAND_str()
#undef XCFG_SFX_DO_EXPAND
};

typedef struct {
  char const *exp;
  regex_t     reg;
} load_reg;

typedef enum {
  LOAD_REG_sxx,
  LOAD_REG_uxx,
  LOAD_REG_fxx,
  LOAD_REG_str,
  LOAD_REG_COUNT,
} load_reg_type;

static load_reg load_reg_lookup[LOAD_REG_COUNT] = {
  [LOAD_REG_sxx] = { .exp = "^[-+]?([0-9]+|0x[a-fA-F0-9]+)$" },
  [LOAD_REG_uxx] = { .exp = "^([0-9]+|0x[a-fA-F0-9]+)$"      },
  [LOAD_REG_fxx] = { .exp = "^[-+]?[0-9]+(\\.[0-9]+)?$"      },
  [LOAD_REG_str] = { .exp = "^\".*\"$"                       },
};

static bool
load_reg_match(load_reg_type type, xcfg_str vstr)
{
  load_reg  *r = &load_reg_lookup[type];
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
    r = &load_reg_lookup[i];
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
    r = &load_reg_lookup[i];
    regfree(&r->reg);
  }

  free(*(xcfg_str *)(load_tmp[XCFG_TID_str]));
}

#define XCFG_TYPE_DO_LOAD(sfx) CONCATENATE(XCFG_TYPE(sfx), _load)

#define XCFG_SFX_DO_EXPAND(sfx, regtype, convert) \
  static bool \
  XCFG_TYPE_DO_LOAD(sfx)(xcfg_str vstr, xcfg_ptr pval) \
  { \
    int base = 10; \
    \
    if (!load_reg_match(regtype, vstr)) \
      return false; \
    \
    if (!strncmp(vstr, "0x",  2)) { \
      vstr += 2; \
      base = 16; \
    } \
    \
    *XCFG_TYPE_CAST(sfx, pval) = convert(vstr, NULL, base); \
    return true; \
  }

  XCFG_SFX_EXPAND_sxx(LOAD_REG_sxx, strtoimax)
  XCFG_SFX_EXPAND_uxx(LOAD_REG_uxx, strtoumax)
#undef XCFG_SFX_DO_EXPAND

static bool
XCFG_TYPE_DO_LOAD(XCFG_SFX_f32)(xcfg_str vstr, xcfg_ptr pval)
{
  if (!load_reg_match(LOAD_REG_fxx, vstr))
    return false;

  *XCFG_TYPE_CAST(XCFG_SFX_f32, pval) = strtof(vstr, NULL);
  return true;
}

static bool
XCFG_TYPE_DO_LOAD(XCFG_SFX_f64)(xcfg_str vstr, xcfg_ptr pval)
{
  if (!load_reg_match(LOAD_REG_fxx, vstr))
    return false;

  *XCFG_TYPE_CAST(XCFG_SFX_f64, pval) = strtod(vstr, NULL);
  return true;
}

static bool
XCFG_TYPE_DO_LOAD(XCFG_SFX_str)(xcfg_str vstr, xcfg_ptr pval)
{
  xcfg_str vtmp;
  xcfg_u32 vlen;

  if (!load_reg_match(LOAD_REG_str, vstr))
    return false;

  vlen = strlen(vstr) - (/*quotes */ 2);
  vtmp = calloc(vlen + 1, sizeof *vtmp);
  if (!vtmp)
    return false;

  /* fill allocated loaded string */
  memcpy(vtmp, vstr + 1, vlen);

  /* return address of loaded string */
  *XCFG_TYPE_CAST(XCFG_SFX_str, pval) = vtmp;
  return true;
}

static xcfg_type_ops type_ops[XCFG_TID_COUNT] = {
  /* `XCFG_SFX_ptr` is a runtime-only feature: no support for save/load */

#define XCFG_SFX_DO_EXPAND(sfx) \
  [XCFG_TID(sfx)] = { XCFG_TYPE_DO_SAVE(sfx), XCFG_TYPE_DO_LOAD(sfx) },

  XCFG_SFX_EXPAND_val()
  XCFG_SFX_EXPAND_str()
#undef XCFG_SFX_DO_EXPAND
};

void
xcfg_file_dispose(xcfg_file *file)
{
  if (!file)
    return;

  free(file->path);
}

xcfg_ret
xcfg_file_set_path(xcfg_file *file, xcfg_str path)
{
  file->path = strdup(path);
  return XCFG_RET_SUCCESS;
}

static xcfg_ret
xcfg_node_tvs_do_save(xcfg_node_tvs *curr, xcfg_ptr context)
{
  save_ctx  *save = context;
  xcfg_node *node = curr->node;
  xcfg_ptr   pfld;

  if (!node || !save->data)
    return XCFG_RET_FAILRUE;

  pfld = xcfg_node_get_fld_ptr(node, save->data);
  if (!pfld)
    return XCFG_RET_FAILRUE;

  if (node->rtfi->tid != XCFG_TID_obj) {
    xcfg_type_ops *tops = &type_ops[node->rtfi->tid];
    xcfg_str       vstr;

    if (tops->save && tops->save(pfld, &vstr)) {
      fprintf(save->stream, INDENT(".%s = %s,\n"), INDENTARG(curr->depth + 1), node->rtfi->key, vstr);
      free(vstr);
    }
  } else {
    /* push stack */
    fprintf(save->stream, INDENT(".%s = {\n"), INDENTARG(curr->depth + 1), node->rtfi->key);
  }

  if (curr->meta.bound_N) {
    /* pop stack  */
    fprintf(save->stream, INDENT("},\n"), INDENTARG(curr->depth));
  }

  return XCFG_RET_SUCCESS;
}

xcfg_ret
xcfg_file_save(xcfg_file *file, xcfg_tree *tree, xcfg_ptr data)
{
  save_ctx save;

  if (!file->path || !(*file->path))
    return XCFG_RET_INVALID;

  save.data = data;
  save.stream = fopen(file->path, "w");
  if (!save.stream)
    return XCFG_RET_INVALID;

  fseek(save.stream, 0, SEEK_SET);
  { /* save header */
    fprintf(save.stream, "0x%x\n",   tree->rtti->size);
    fprintf(save.stream, "(%s) {\n", tree->rtti->name);
  }
  { /* save fields */
    xcfg_tree_tvs_depth_first(tree, xcfg_node_tvs_do_save, &save);
  }
  fclose(save.stream);

  return XCFG_RET_SUCCESS;
}

#define logiload(path, line, fmt, args...) \
  logi("load[%s:%u] " fmt, (path), (xcfg_u32)(line), ## args)

xcfg_ret
xcfg_file_load(xcfg_file *file, xcfg_tree *tree, xcfg_ptr data)
{
  load_ctx load;
  xcfg_str path;
  char     rbuf[1024];
  xcfg_u32 rlen = sizeof rbuf - 1;
  xcfg_u32 line;
  xcfg_str t0, t1;
  xcfg_ret ret = XCFG_RET_FAILRUE;

  path = file->path;
  if (!path || !(*path))
    return XCFG_RET_INVALID;

  load.data = data;
  load.stream = fopen(path, "r");
  if (!load.stream)
    return XCFG_RET_INVALID;

  memset(rbuf, 0, sizeof rbuf);
  line = 0;

  fseek(load.stream, 0, SEEK_SET);
  { /* load header */
    xcfg_u32 type_size;

    if (line++, !fgets(rbuf, rlen, load.stream))
      goto out;

    // xcfg_u32_load(t0, &type_size);
    t0 = rbuf;
    if (strncmp(t0, "0x", 2))
      goto out;
    t0 += 2;

    type_size = strtoul(t0, NULL, 16);
    if (type_size != tree->rtti->size) {
      logiload(path, line, "cfg type size is invalid: %u != %u", type_size, tree->rtti->size);
      goto out;
    }
    logiload(path, line, "cfg type size: %u", type_size);

    memset(rbuf, 0, sizeof rbuf);
    if (line++, !fgets(rbuf, rlen, load.stream))
      goto out;

    t0 = strchr(rbuf, '(');
    t1 = strchr(rbuf, ')');
    if (!t0 || !t1 || t0 > t1)
      goto out;

    t0++;
    while (t0 < t1 && isspace(*t0)) ++t0;
    if (t0 == t1) goto out;
    t1--;
    while (t1 > t0 && isspace(*t1)) --t1;
    if (t0 == t1) goto out;
    *(t1 + 1) = '\0';
    /* here `t0` is a valid type name string */

    if (strcmp(t0, tree->rtti->name)) {
      logiload(path, line, "cfg type name is invalid: '%s' != '%s'", t0, tree->rtti->name);
      goto out;
    }
    logiload(path, line, "cfg type name: %s", t0);
  }
  { /* load fields */
    xcfg_node *base = &tree->root;

    memset(rbuf, 0, sizeof rbuf);
    while (line++, fgets(rbuf, rlen, load.stream) && base) {
      xcfg_node     *node;
      xcfg_type_ops *tops;
      xcfg_ptr       vtmp;
      xcfg_str       eq;
      xcfg_u32       i;

      t0 = rbuf;
      while (*t0 && isspace(*t0)) ++t0;

      /* skip commented lines */
      if (*t0 == '#') {
        logiload(path, line, "commented");
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
      while (t1 > t0 && isspace(*t1)) --t1;
      if (t0 == t1) continue;
      *(t1 + 1) = '\0';
      /* `t0` is a valid `key` here */

      /* search for node with key */
      for (i = 0; i < base->nnext; ++i) {
        node = base->next[i];
        if (!strncmp(t0, node->rtfi->key, strlen(node->rtfi->key)))
          break;
      }

      if (i == base->nnext) {
        logiload(path, line, "unknown key: %s", t0);
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
      while (t1 > t0 && isspace(*t1)) --t1;
      if (t0 == t1) continue;
      *(t1 + 1) = '\0';
      /* `t0` is a valid `value` here */

      tops = &type_ops[node->rtfi->tid];
      vtmp =  load_tmp[node->rtfi->tid];
      if (!tops->load || !vtmp) {
        logiload(path, line, "load function not found for: %s", node->rtfi->key);
        continue;
      }

      logiload(path, line, "loading: %s = %s", node->rtfi->key, t0);
      if (!tops->load(t0, vtmp)) {
        logiload(path, line, "failed to load value for: %s", node->rtfi->key);
        continue;
      }

      ret = xcfg_node_set_value(node, data, vtmp);
      if (ret < 0) {
        logiload(path, line, "failed to set value for: %s", node->rtfi->key);
        continue;
      }
    }
  }
  ret = XCFG_RET_SUCCESS;

out:
  fclose(load.stream);
  return ret;
}