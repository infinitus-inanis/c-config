#include "cfg-base.h"

#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <stddef.h>
#include <stdio.h>

#include <ctype.h>
#include <inttypes.h>

#define logi(fmt, args...) printf("[cfg]: "fmt "\n", ## args)

cfg_ret
cfg_ent_set_generic(void          *cfg,
                    cfg_ent const *ent,
                    void    const *val,
                    cfg_upd const  upd_flag)
{
  void          *dst = (void    *)((cfg_u08 *)(cfg) + ent->fld.own.offset);
  cfg_upd       *upd = (cfg_upd *)((cfg_u08 *)(cfg) + ent->fld.upd.offset);
  void    const *src;
  bool           cmp;

  switch (ent->fld.own.type) {
    case CFG_FLD_TYPE_NAME(CFG_TID_s08): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_s16): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_s32): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_s64): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_u08): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_u16): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_u32): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_u64): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_f32): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_f64): {
      /* got pointer to basic value from op
         need to copy value pointed to by 'val' */
      src = val;

      /* basic value requires comparison */
      cmp = true;
    } break;

    case CFG_FLD_TYPE_NAME(CFG_TID_ptr): {
      /* got pointer to pointer from op
         need to copy address pointed to by 'val' */
      src = val;

      /* no reason to compare pointers */
      cmp = false;
    } break;

    case CFG_FLD_TYPE_NAME(CFG_TID_obj): {
      /* got pointer to pointer to object from op
         need to copy value pointed to by dereferenced 'val' */
      src = *(cfg_obj *)(val);

      /* object value requires comparison */
      cmp = true;
    } break;

    default:
      return CFG_RET_INVALID;
  }

  if (cmp && !memcmp(dst, src, ent->fld.own.size))
    return CFG_RET_SUCCESS;

  memcpy(dst, src, ent->fld.own.size);
  *upd |= upd_flag;

  return CFG_RET_SUCCESS;
}

cfg_ret
cfg_ent_get_generic(void    const *cfg,
                    cfg_ent const *ent,
                    void          *val)
{
  void const *src = (cfg_u08 *)(cfg) + ent->fld.own.offset;
  void       *dst;

  switch (ent->fld.own.type) {
    case CFG_FLD_TYPE_NAME(CFG_TID_s08): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_s16): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_s32): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_s64): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_u08): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_u16): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_u32): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_u64): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_f32): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_f64): {
      dst = val;
    } break;

    case CFG_FLD_TYPE_NAME(CFG_TID_ptr): {
      dst = val;
    } break;

    case CFG_FLD_TYPE_NAME(CFG_TID_obj): {
      dst = *(cfg_obj **)(val);
    } break;

    default:
      return CFG_RET_INVALID;
  }

  memcpy(dst, src, ent->fld.own.size);

  return CFG_RET_SUCCESS;
}

static cfg_ret
cfg_ent_req_approve(cfg_ent const *ent, cfg_fld_type const req_type)
{
  cfg_fld_type const fld_type = ent->fld.own.type;
  cfg_u32      const fld_size = ent->fld.own.size;

  switch (req_type) {
    case CFG_FLD_TYPE_NAME(CFG_TID_s08): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_u08): {
      if (fld_size != sizeof(cfg_s08))
        return CFG_RET_INVALID;
    } break;

    case CFG_FLD_TYPE_NAME(CFG_TID_s16): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_u16): {
      if (fld_size != sizeof(cfg_s16))
        return CFG_RET_INVALID;
    } break;

    case CFG_FLD_TYPE_NAME(CFG_TID_s32): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_u32): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_f32): {
      if (fld_size != sizeof(cfg_s32))
        return CFG_RET_INVALID;
    } break;

    case CFG_FLD_TYPE_NAME(CFG_TID_s64): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_u64): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_f64):{
      if (fld_size != sizeof(cfg_s64))
        return CFG_RET_INVALID;
    } break;

    case CFG_FLD_TYPE_NAME(CFG_TID_ptr): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_obj): {
      if (fld_type != req_type)
        return CFG_RET_INVALID;
    } break;

    default:
      return CFG_RET_UNKNOWN;
  }
  return CFG_RET_SUCCESS;
}


#define PATH_MAX 4096

typedef struct {
  char path[PATH_MAX];

} cfg_file;

struct cfg_ctx {
  void           *data;
  cfg_ent const **ents;
  cfg_u32         ents_cnt;

  cfg_file        file;
};

cfg_ctx_t *
cfg_ctx_create(void           *data,
               cfg_ent const **ents,
               cfg_u32 const   ents_cnt)
{
  cfg_ctx_t *ctx;

  ctx = calloc(1, sizeof *ctx);
  if (!ctx)
    return NULL;

  ctx->data = data;
  ctx->ents = ents;
  ctx->ents_cnt = ents_cnt;

  return ctx;
}

void
cfg_ctx_destroy(cfg_ctx_t *ctx)
{
  if (!ctx)
    return;

  free(ctx);
}

static cfg_ent const *
cfg_ctx_select_ent_by_key(cfg_ctx_t const *ctx, char const *key)
{
  cfg_u32 ent_i;

  /* TODO (butsuk_d):
     optimize this using radix-tree */

  for (ent_i = 0; ent_i < ctx->ents_cnt; ++ent_i) {
    cfg_ent const *ent = ctx->ents[ent_i];

    if (!strcmp(key, ent->key))
      return ent;
  }

  return NULL;
}

static cfg_ent const *
cfg_ctx_select_ent_by_ref(cfg_ctx_t const *ctx, void const *fld)
{
  cfg_u32 fld_offset;
  cfg_u32 ent_i;

  /* TODO (butsuk_d):
     optimize this using hashtable */

  if (fld < ctx->data)
    return NULL;

  fld_offset = (cfg_u08 *)(fld) - (cfg_u08 *)(ctx->data);

  for (ent_i = 0; ent_i < ctx->ents_cnt; ++ent_i) {
    cfg_ent const *ent = ctx->ents[ent_i];

    if (ent->fld.own.offset == fld_offset)
      return ent;
  }

  return NULL;
}

cfg_ret
cfg_ctx_file_bind(cfg_ctx_t *ctx, char const *path)
{
  /* TODO (butsuk_d): make it safe */
  strncpy(ctx->file.path, path, sizeof ctx->file.path);
  return CFG_RET_SUCCESS;
}

#define CFG_CTX_FLD(ctx, ent, type) \
  (type *)((cfg_u08 *)(ctx)->data + (ent)->fld.own.offset)

#define CFG_ENT_SEP '='

cfg_ret
cfg_ctx_file_save(cfg_ctx_t *ctx)
{
  FILE    *file;
  cfg_u32  ent_i;

  if (ctx->file.path[0] == '\0')
    return CFG_RET_INVALID;

  file = fopen(ctx->file.path, "w");
  fseek(file, 0, SEEK_SET);

  for (ent_i = 0; ent_i < ctx->ents_cnt; ++ent_i) {
    cfg_ent const *ent = ctx->ents[ent_i];

    if (!ent->key)
      continue;

    #define CFG_FLD_SAVE(tid, fmt)                              \
      case CFG_FLD_TYPE_NAME(tid): {                            \
        fprintf(file, "%s %c " fmt "\n", ent->key, CFG_ENT_SEP, \
          *CFG_CTX_FLD(ctx, ent, CFG_TYPE(tid)));               \
      } break

    switch (ent->fld.own.type) {
      CFG_FLD_SAVE(CFG_TID_s08, "%"PRId8);
      CFG_FLD_SAVE(CFG_TID_s16, "%"PRId16);
      CFG_FLD_SAVE(CFG_TID_s32, "%"PRId32);
      CFG_FLD_SAVE(CFG_TID_s64, "%"PRId64);
      CFG_FLD_SAVE(CFG_TID_u08, "%"PRIu8);
      CFG_FLD_SAVE(CFG_TID_u16, "%"PRIu16);
      CFG_FLD_SAVE(CFG_TID_u32, "%"PRIu32);
      CFG_FLD_SAVE(CFG_TID_u64, "%"PRIu64);
      CFG_FLD_SAVE(CFG_TID_f32, "%f");
      CFG_FLD_SAVE(CFG_TID_f64, "%lf");

      case CFG_FLD_TYPE_NAME(CFG_TID_ptr): /* fallthrough */
        /* no reason to write pointers it's runtime-only feature */

      case CFG_FLD_TYPE_NAME(CFG_TID_obj): /* fallthrough */
        /* no support for object serialization yet */

      default:
        /* ingore invalid entries */
        continue;
    }
  }

  fclose(file);

  return CFG_RET_SUCCESS;
}

static cfg_ret
cfg_ctx_file_line_proc(char *line, cfg_u32 count, char **pkey, char **pval)
{
  #define is_nul_char(c) ((c) == '\0')
  #define is_com_char(c) ((c) == '#')
  #define is_sok_char(c) ((c) == '_' || isalpha(c))
  #define is_kvp_char(c) ((c) == '_' || (c) == '.' || (c) == '-' || isalnum(c))
  #define is_sep_char(c) ((c) == CFG_ENT_SEP)

  char *tmp;

  if (!line)
    return CFG_RET_INVALID;

  tmp = line;
  /* ignore noise */
  while (!is_sok_char(*tmp)) {
    if (is_nul_char(*tmp))
      return CFG_RET_FAILRUE;

    if (is_com_char(*tmp))
      return CFG_RET_FAILRUE;

    tmp++;
  }

  /* found start of a key */
  *pkey = tmp;

  do { /* ignore key body */
    tmp++;
    if (is_nul_char(*tmp)) {
      logi("load: invalid line at %u (not an entry)", count);
      return CFG_RET_FAILRUE;
    }
  } while (is_kvp_char(*tmp));

  /* mend end-of-key termination */
  *tmp = '\0';

  do { /* search kvp separator */
    tmp++;
    if (is_nul_char(*tmp)) {
      logi("load: invalid line at %u (not an entry)", count);
      return CFG_RET_FAILRUE;
    }
  } while (!is_sep_char(*tmp));

  do { /* ignore noise */
    tmp++;
    if (is_nul_char(*tmp)) {
      logi("load: invalid line at %u (no-val entry)", count);
      return CFG_RET_FAILRUE;
    }
  } while (!is_kvp_char(*tmp));

  /* found start of a val */
  *pval = tmp;

  do { /* ignore val body */
    tmp++;
    if (is_nul_char(*tmp))
      return CFG_RET_SUCCESS;

  } while (is_kvp_char(*tmp));

  /* mend end-of-val termination */
  *tmp = '\0';

  return CFG_RET_SUCCESS;
}

cfg_ret
cfg_ctx_file_load(cfg_ctx_t *ctx)
{
  FILE   *file;
  char    line[1024];
  cfg_u32 count;
  cfg_ret ret;

  if (ctx->file.path[0] == '\0')
    return CFG_RET_INVALID;

  file = fopen(ctx->file.path, "r");
  if (!file)
    return CFG_RET_INVALID;

  fseek(file, 0, SEEK_SET);

  memset(line, 0, sizeof line);
  count = 0;

  while (++count, fgets(line, sizeof line - 1, file)) {
    cfg_ent const *ent;
    char          *key;
    char          *val;

    ret = cfg_ctx_file_line_proc(line, count, &key, &val);
    if (ret != CFG_RET_SUCCESS)
      continue;

    // logi("load: found entity: %s = %s", key, val);

    ent = cfg_ctx_select_ent_by_key(ctx, key);
    if (!ent) {
      logi("load: unknown entity at line %u", count);
      continue;
    }

    #define CFG_FLD_LOAD(tid, fmt)                              \
      case CFG_FLD_TYPE_NAME(tid): {                            \
        sscanf(val, fmt, CFG_CTX_FLD(ctx, ent, CFG_TYPE(tid))); \
      } break

    switch (ent->fld.own.type) {
      CFG_FLD_LOAD(CFG_TID_s08, "%"SCNd8);
      CFG_FLD_LOAD(CFG_TID_s16, "%"SCNd16);
      CFG_FLD_LOAD(CFG_TID_s32, "%"SCNd32);
      CFG_FLD_LOAD(CFG_TID_s64, "%"SCNd64);
      CFG_FLD_LOAD(CFG_TID_u08, "%"SCNu8);
      CFG_FLD_LOAD(CFG_TID_u16, "%"SCNu16);
      CFG_FLD_LOAD(CFG_TID_u32, "%"SCNu32);
      CFG_FLD_LOAD(CFG_TID_u64, "%"SCNu64);
      CFG_FLD_LOAD(CFG_TID_f32, "%f");
      CFG_FLD_LOAD(CFG_TID_f64, "%lf");

      case CFG_FLD_TYPE_NAME(CFG_TID_ptr):
        /* no reason to write pointers it's runtime-only feature */
        /* fallthrough */

      case CFG_FLD_TYPE_NAME(CFG_TID_obj):
        /* no support for object serialization yet */
        /* fallthrough */

      default:
        /* ingore invalid entries */
        continue;
    }
  }

  fclose(file);

  return CFG_RET_SUCCESS;
}


#define CFG_ENT_SET(tid, ent) \
  ((DEFINE_CFG_ENT_OP_SET((*), tid))(ent)->ops.set)

#define CFG_ENT_GET(tid, ent) \
  ((DEFINE_CFG_ENT_OP_GET((*), tid))(ent)->ops.get)

#define DECLARE_CFG_CTX_OP_SET(tid)                         \
  DEFINE_CFG_CTX_OP_SET(tid)                                \
  {                                                         \
    cfg_ent const *ent;                                     \
    cfg_ret        ret;                                     \
                                                            \
    if (!ctx || !key)                                       \
      return CFG_RET_INVALID;                               \
                                                            \
    ent = cfg_ctx_select_ent_by_key(ctx, key);              \
    if (!ent)                                               \
      return CFG_RET_UNKNOWN;                               \
                                                            \
    ret = cfg_ent_req_approve(ent, CFG_FLD_TYPE_NAME(tid)); \
    if (ret != CFG_RET_SUCCESS)                             \
      return ret;                                           \
                                                            \
    ret = CFG_ENT_SET(tid, ent)(ctx->data, ent, val);       \
    return ret;                                             \
  }

#define DECLARE_CFG_CTX_OP_GET(tid)                         \
  DEFINE_CFG_CTX_OP_GET(tid)                                \
  {                                                         \
    cfg_ent const *ent;                                     \
    cfg_ret        ret;                                     \
                                                            \
    if (!ctx || !key)                                       \
      return CFG_RET_INVALID;                               \
                                                            \
    ent = cfg_ctx_select_ent_by_key(ctx, key);              \
    if (!ent)                                               \
      return CFG_RET_UNKNOWN;                               \
                                                            \
    ret = cfg_ent_req_approve(ent, CFG_FLD_TYPE_NAME(tid)); \
    if (ret != CFG_RET_SUCCESS)                             \
      return ret;                                           \
                                                            \
    ret = CFG_ENT_GET(tid, ent)(ctx->data, ent, val);       \
    return ret;                                             \
  }

EXPAND_CFG_TIDS(DECLARE_CFG_CTX_OP_SET, EOL_EMPTY)
EXPAND_CFG_TIDS(DECLARE_CFG_CTX_OP_GET, EOL_EMPTY)


#define DECLARE_CFG_CTX_OP_SET_BY_REF(tid)                  \
  DEFINE_CFG_CTX_OP_SET_BY_REF(tid)                         \
  {                                                         \
    cfg_ent const *ent;                                     \
    cfg_ret        ret;                                     \
                                                            \
    if (!ctx || !fld)                                       \
      return CFG_RET_INVALID;                               \
                                                            \
    ent = cfg_ctx_select_ent_by_ref(ctx, fld);              \
    if (!ent)                                               \
      return CFG_RET_UNKNOWN;                               \
                                                            \
    ret = cfg_ent_req_approve(ent, CFG_FLD_TYPE_NAME(tid)); \
    if (ret != CFG_RET_SUCCESS)                             \
      return ret;                                           \
                                                            \
    ret = CFG_ENT_SET(tid, ent)(ctx->data, ent, val);       \
    return ret;                                             \
  }

EXPAND_CFG_TIDS(DECLARE_CFG_CTX_OP_SET_BY_REF, EOL_EMPTY)