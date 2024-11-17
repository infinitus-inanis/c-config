#define _GNU_SOURCE /* for asprintf */

#include "cfg-base.h"
#include "utils/list.h"

#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <stddef.h>
#include <stdio.h>

#include <ctype.h>
#include <inttypes.h>

#define logi(fmt, args...) printf("[cfg]: "fmt "\n", ## args)

typedef struct cfg_node cfg_node;
struct cfg_node {
  struct list   self;  /* node in list of all nodes */
  cfg_node     *owner; /* pointer to predecessor    */
  char         *path;
  cfg_fld_info  fld;
  cfg_upd_info  upd;
};

#define CFG_NODE_PTR(cfg, off, type) \
  (type *)((cfg_u08 *)(cfg) + (cfg_u32)(off))

#define CFG_NODE_FLD_PTR(cfg, node, type) \
  CFG_NODE_PTR(cfg, (node)->fld.offset, type)

#define CFG_NODE_UPD_PTR(cfg, node) \
  CFG_NODE_PTR(cfg, (node)->upd.offset, cfg_upd)

inline static void
cfg_node_clear_upd(void           *cfg,
                   cfg_node const *node)
{
  *CFG_NODE_UPD_PTR(cfg, node) &= ~CFG_UPD(node->upd.id);
}

static void
cfg_node_raise_upd(void           *cfg,
                   cfg_node const *node)
{
  do {
    *CFG_NODE_UPD_PTR(cfg, node) |= CFG_UPD(node->upd.id);
  } while ((node = node->owner));
}

static cfg_ret
cfg_node_set_value(void           *cfg,
                   cfg_node const *node,
                   void     const *pval)
{
  void       *dst = CFG_NODE_FLD_PTR(cfg, node, void);
  void const *src;
  bool        cmp;

  switch (node->fld.type) {
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
         need to copy value pointed to by 'value' */
      src = pval;

      /* basic value requires comparison */
      cmp = true;
    } break;

    case CFG_FLD_TYPE_NAME(CFG_TID_ptr): {
      /* got pointer to pointer from op
         need to copy address pointed to by 'value' */
      src = pval;

      /* no reason to compare pointers */
      cmp = false;
    } break;

    case CFG_FLD_TYPE_NAME(CFG_TID_obj): {
      /* got pointer to pointer to object from op
         need to copy value pointed to by dereferenced 'value' */
      src = *(cfg_obj *)(pval);

      /* object value requires comparison */
      cmp = true;
    } break;

    default:
      return CFG_RET_INVALID;
  }

  if (cmp && !memcmp(dst, src, node->fld.size))
    return CFG_RET_SUCCESS;

  memcpy(dst, src, node->fld.size);
  cfg_node_raise_upd(cfg, node);

  return CFG_RET_SUCCESS;
}

cfg_ret
cfg_node_get_value(void     const *cfg,
                   cfg_node const *node,
                   void           *pval)
{
  void const *src = CFG_NODE_FLD_PTR(cfg, node, void);
  void       *dst;

  switch (node->fld.type) {
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
      dst = pval;
    } break;

    case CFG_FLD_TYPE_NAME(CFG_TID_ptr): {
      dst = pval;
    } break;

    case CFG_FLD_TYPE_NAME(CFG_TID_obj): {
      dst = *(cfg_obj **)(pval);
    } break;

    default:
      return CFG_RET_INVALID;
  }

  memcpy(dst, src, node->fld.size);

  return CFG_RET_SUCCESS;
}

static cfg_ret
cfg_node_req_check(cfg_node     const *node,
                   cfg_fld_type const  req_type)
{
  cfg_fld_type const fld_type = node->fld.type;
  cfg_u32      const fld_size = node->fld.size;

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
  void        *data;
  cfg_u32      size;
  struct list  tree;
  cfg_file     file;
};

static cfg_node *
cfg_node_reify(cfg_info *info,
               cfg_node *owner)
{
  cfg_node *node = calloc(1, sizeof *node);
  cfg_u32   i;

  node->owner = owner;
  node->fld   = info->fld;
  node->upd   = info->upd;

  if (!owner) {
    asprintf(&node->path, "%s", info->key);
  } else {
    asprintf(&node->path, "%s.%s", owner->path, info->key);
    node->fld.offset += owner->fld.offset;
    node->upd.offset += owner->fld.offset;
  }

  if (!info->children.size
   || !info->children.data
  ) return node;

  list_init(&node->self);
  for (i = 0; i < info->children.size; ++i) {
    cfg_info *cinfo = &info->children.data[i];
    cfg_node *cnode = cfg_node_reify(cinfo, node);
    list_insert_tail(&cnode->self, &node->self);
  }

  return node;
}

static void
cfg_ctx_dump_nodes(cfg_ctx *ctx) {
  cfg_node *node, *temp;

  logi("ctx: dump");
  list_for_each_data(node, &ctx->tree, self) {
    logi("  '%s' (%p) (%p)",
      node->path,
      CFG_NODE_FLD_PTR(ctx->data, node, void),
      CFG_NODE_UPD_PTR(ctx->data, node));
    logi("    fld.type:   %d", node->fld.type);
    logi("    fld.size:   %u", node->fld.size);
    logi("    fld.offset: %u", node->fld.offset);
    logi("    upd.id:     %u", node->upd.id);
    logi("    upd.size:   %u", node->upd.size);
    logi("    upd.offset: %u", node->upd.offset);

    logi("    deps:");
    temp = node->owner;
    while (temp) {
      logi("      '%s'", temp->path);
      temp = temp->owner;
    }
  }
}

cfg_ctx *
cfg_ctx_create(void     *data,
               cfg_u32   size,
               cfg_info *infos,
               cfg_u32   infos_n)
{
  cfg_ctx *ctx;
  cfg_u32  i;

  ctx = calloc(1, sizeof *ctx);
  if (!ctx)
    return NULL;

  ctx->data = data;
  ctx->size = size;

  list_init(&ctx->tree);
  for (i = 0; i < infos_n; ++i) {
    cfg_info *info = &infos[i];
    cfg_node *node = cfg_node_reify(info, NULL);
    list_insert_tail(&node->self, &ctx->tree);
  }

  return ctx;
}

void
cfg_ctx_destroy(cfg_ctx *ctx)
{
  cfg_node *node, *temp;

  if (!ctx)
    return;

  list_for_each_data_safe(node, temp, &ctx->tree, self) {
    list_unlink(&node->self);
    free(node->path);
    free(node);
  }

  free(ctx);
}

static cfg_node const *
cfg_ctx_get_node_by_key(cfg_ctx const *ctx, char const *key)
{
  /* TODO (butsuk_d):
     optimize this using radix-tree */

  cfg_node const *node;

  list_for_each_data(node, &ctx->tree, self) {
    if (!strcmp(key, node->path))
      return node;
  };

  return NULL;
}

static cfg_node const *
cfg_ctx_get_node_by_ref(cfg_ctx const *ctx, void const *fld)
{
  /* TODO (butsuk_d):
     optimize this using hashtable */

  cfg_node const *node;
  cfg_u32         fld_offset;

  if (fld < ctx->data)
    return NULL;

  fld_offset = (cfg_u08 *)(fld) - (cfg_u08 *)(ctx->data);
  if (fld_offset >= ctx->size)
    return NULL;

  list_for_each_data(node, &ctx->tree, self) {
    if (fld_offset == node->fld.offset)
      return node;
  };

  return NULL;
}

cfg_ret
cfg_ctx_bind_file(cfg_ctx *ctx, char const *path)
{
  strncpy(ctx->file.path, path, sizeof ctx->file.path);
  return CFG_RET_SUCCESS;
}

#define CFG_CTX_FLD(ctx, node, type) \
  (type *)((cfg_u08 *)(ctx)->data + (node)->fld.offset)

#define CFG_ENT_SEP '='

cfg_ret
cfg_ctx_save_file(cfg_ctx *ctx)
{
  FILE     *file;
  cfg_node *node;

  if (ctx->file.path[0] == '\0')
    return CFG_RET_INVALID;

  file = fopen(ctx->file.path, "w");
  fseek(file, 0, SEEK_SET);

  list_for_each_data(node, &ctx->tree, self) {
    if (!node->path)
      continue;

    #define CFG_FLD_SAVE(tid, fmt)                                \
      case CFG_FLD_TYPE_NAME(tid): {                              \
        fprintf(file, "%s %c " fmt "\n", node->path, CFG_ENT_SEP, \
          *CFG_NODE_FLD_PTR(ctx->data, node, CFG_TYPE(tid)));     \
      } break

    switch (node->fld.type) {
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

      case CFG_FLD_TYPE_NAME(CFG_TID_ptr):
        /* ignore ptrs (runtime-only feature) */

      case CFG_FLD_TYPE_NAME(CFG_TID_obj):
        /* ignore objs (runtime-only feature) */

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
cfg_ctx_load_file(cfg_ctx *ctx)
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
    cfg_node const *node;
    char           *key;
    char           *val;

    ret = cfg_ctx_file_line_proc(line, count, &key, &val);
    if (ret != CFG_RET_SUCCESS)
      continue;

    node = cfg_ctx_get_node_by_key(ctx, key);
    if (!node) {
      logi("load: unknown entity at line %u", count);
      continue;
    }

    #define CFG_FLD_LOAD(tid, fmt)                           \
      case CFG_FLD_TYPE_NAME(tid): {                         \
        CFG_TYPE(tid) tmp;                                   \
        sscanf(val, fmt, &tmp);                              \
        cfg_node_set_value(ctx->data, node, (void *)(&tmp)); \
      } break

    switch (node->fld.type) {
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
        /* ignore ptrs (runtime-only feature) */

      case CFG_FLD_TYPE_NAME(CFG_TID_obj):
        /* ignore objs (runtime-only feature) */

      default:
        /* ingore invalid entries */
        continue;
    }
  }

  fclose(file);

  return CFG_RET_SUCCESS;
}


#define DECLARE_CFG_CTX_OP_SET(tid)                             \
  DEFINE_CFG_CTX_OP_SET(tid)                                    \
  {                                                             \
    cfg_node const *node;                                       \
    cfg_ret         ret;                                        \
                                                                \
    if (!ctx || !key)                                           \
      return CFG_RET_INVALID;                                   \
                                                                \
    node = cfg_ctx_get_node_by_key(ctx, key);                   \
    if (!node)                                                  \
      return CFG_RET_UNKNOWN;                                   \
                                                                \
    ret = cfg_node_req_check(node, CFG_FLD_TYPE_NAME(tid));     \
    if (ret != CFG_RET_SUCCESS)                                 \
      return ret;                                               \
                                                                \
    ret = cfg_node_set_value(ctx->data, node, (void *)(&val));  \
    return ret;                                                 \
  }

#define DECLARE_CFG_CTX_OP_GET(tid)                             \
  DEFINE_CFG_CTX_OP_GET(tid)                                    \
  {                                                             \
    cfg_node const *node;                                       \
    cfg_ret        ret;                                         \
                                                                \
    if (!ctx || !key)                                           \
      return CFG_RET_INVALID;                                   \
                                                                \
    node = cfg_ctx_get_node_by_key(ctx, key);                   \
    if (!node)                                                  \
      return CFG_RET_UNKNOWN;                                   \
                                                                \
    ret = cfg_node_req_check(node, CFG_FLD_TYPE_NAME(tid));     \
    if (ret != CFG_RET_SUCCESS)                                 \
      return ret;                                               \
                                                                \
    ret = cfg_node_get_value(ctx->data, node, (void *)(val));   \
    return ret;                                                 \
  }

EXPAND_CFG_TIDS(DECLARE_CFG_CTX_OP_SET, EOL_EMPTY)
EXPAND_CFG_TIDS(DECLARE_CFG_CTX_OP_GET, EOL_EMPTY)


#define DECLARE_CFG_CTX_OP_SET_BY_REF(tid)                      \
  DEFINE_CFG_CTX_OP_SET_BY_REF(tid)                             \
  {                                                             \
    cfg_node const *node;                                       \
    cfg_ret        ret;                                         \
                                                                \
    if (!ctx || !fld)                                           \
      return CFG_RET_INVALID;                                   \
                                                                \
    node = cfg_ctx_get_node_by_ref(ctx, fld);                   \
    if (!node)                                                  \
      return CFG_RET_UNKNOWN;                                   \
                                                                \
    ret = cfg_node_req_check(node, CFG_FLD_TYPE_NAME(tid));     \
    if (ret != CFG_RET_SUCCESS)                                 \
      return ret;                                               \
                                                                \
    ret = cfg_node_set_value(ctx->data, node, (void *)(&val));  \
    return ret;                                                 \
  }

EXPAND_CFG_TIDS(DECLARE_CFG_CTX_OP_SET_BY_REF, EOL_EMPTY)