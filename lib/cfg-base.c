#define _GNU_SOURCE /* for asprintf */

#include "cfg-base.h"
#include "utils/hashtable.h"

#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <stddef.h>
#include <stdio.h>

#include <ctype.h>
#include <inttypes.h>

#define logi(fmt, args...) printf("[cfg]: "fmt "\n", ## args)

int
cfg_info_cmp_by_off(cfg_info const *l, cfg_info const *r) {
  if (l->fld.off < r->fld.off)
    return -1;
  if (l->fld.off > r->fld.off)
    return  1;
  return 0;
}

typedef struct cfg_node cfg_node;
struct cfg_node {
  cfg_str ctx_key;
  cfg_u32 ctx_fld_off;
  cfg_u32 ctx_upd_off;

  cfg_str       key;
  cfg_fld_info  fld;
  cfg_upd_info  upd;

  cfg_node  *prev;
  cfg_node **next;
  cfg_u32    next_n;
};

#define CFG_OFF_PTR(cfg, off, type) \
  (type *)((cfg_u08 *)(cfg) + (cfg_u32)(off))

#define CFG_NODE_FLD_PTR(cfg, node, type) \
  CFG_OFF_PTR(cfg, (node)->ctx_fld_off, type)

#define CFG_NODE_UPD_PTR(cfg, node) \
  CFG_OFF_PTR(cfg, (node)->ctx_upd_off, cfg_upd)

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
  } while ((node = node->prev));
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

    case CFG_FLD_TYPE_NAME(CFG_TID_str): {
      cfg_str src_str = *(cfg_str *)(pval);
      cfg_str dst_str = *(cfg_str *)(dst);
      cfg_u32 src_str_sz = src_str ? strlen(src_str) + (/*null*/ 1) : 0;
      cfg_u32 dst_str_sz = dst_str ? strlen(dst_str) + (/*null*/ 1) : 0;

      if (dst_str_sz == src_str_sz) {
        if (dst_str_sz == 0) {
          return CFG_RET_SUCCESS;
        } else {
          if (!memcmp(dst_str, src_str, dst_str_sz)) {
            return CFG_RET_SUCCESS;
          } else {
            memcpy(dst_str, src_str, dst_str_sz);
            cfg_node_raise_upd(cfg, node);
            return CFG_RET_SUCCESS;
          }
        }
      } else {
        if (src_str_sz == 0) {
          free(dst_str);
          return CFG_RET_SUCCESS;
        } else {
          dst_str = realloc(dst_str, src_str_sz);
          memcpy(dst_str, src_str, src_str_sz);
          memcpy(dst, &dst_str, node->fld.size);
          cfg_node_raise_upd(cfg, node);
          return CFG_RET_SUCCESS;
        }
      }
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
      dst = pval;
    } break;

    case CFG_FLD_TYPE_NAME(CFG_TID_str): {
      cfg_str src_str = *(cfg_str *)(src);
      cfg_str dst_str = *(cfg_str *)(pval);
      cfg_u32 src_str_sz = src_str ? strlen(src_str) + (/*null*/ 1) : 0;

      if (src_str_sz == 0) {
        return CFG_RET_SUCCESS;
      } else {
        dst_str = realloc(dst_str, src_str_sz);
        memcpy(dst_str, src_str, src_str_sz);
        memcpy(pval, &dst_str, node->fld.size);
        return CFG_RET_SUCCESS;
      }
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
    case CFG_FLD_TYPE_NAME(CFG_TID_obj): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_str): {
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

typedef struct {
  cfg_node  root;
  cfg_u32   count;
  ht_ctx   *by_key;
  ht_ctx   *by_off;
} cfg_tree;

typedef bool (* cfg_tree_visit_f)(cfg_node *node, bool bound, cfg_u32 depth, void *data);

typedef struct {
  void    *data;
  bool     bound;
  cfg_u32  depth;
} dfs_node;

void
cfg_tree_dfs(cfg_tree *tree, cfg_tree_visit_f visit, void *data)
{
  dfs_node *stack, curr, *temp;
  cfg_s32   arrow;
  cfg_node *node;
  cfg_u32   i;

  if (tree->count == 0)
    return;

  node = &tree->root;
  if (!node->next_n)
    return;

  stack = calloc(tree->count, sizeof *stack);
  arrow = -1;

  for (i = node->next_n; i > 0; --i) {
    temp = &stack[++arrow];
    temp->data  = node->next[i - 1];
    temp->bound = false;
    temp->depth = 1;
  }

  while (!(arrow < 0)) {
    curr = stack[arrow--];
    node = curr.data;
    if (!visit(node, curr.bound, curr.depth, data))
      break;

    /* NOTE (butsuk_d):
        traverse in reverse because nodes are
        sorted by offset in a natural order */

    if (!node->next_n)
      continue;

    for (i = node->next_n; i > 0; --i) {
      temp = &stack[++arrow];
      temp->data  = node->next[i - 1];
      temp->bound = node->next_n == i;
      temp->depth = curr.depth + 1;
    }
  }

  free(stack);
}

struct cfg_ctx {
  struct {
    void    *ptr;
    cfg_u32  size;
  } cfg;

  cfg_tree tree;
  cfg_file file;
};

static cfg_node *
cfg_tree_node_reify(cfg_info *info,
                    cfg_node *prev,
                    ht_ctx   *by_off,
                    ht_ctx   *by_key)
{
  cfg_node *node = calloc(1, sizeof *node);
  cfg_u32   i;

  node->key = strndup(info->key, 1024);
  node->fld = info->fld;
  node->upd = info->upd;

  node->prev = prev;
  if (!node->prev) {
    asprintf(&node->ctx_key, "%s", node->key);
    node->ctx_fld_off = node->fld.off;
    node->ctx_upd_off = node->upd.off;
  } else {
    asprintf(&node->ctx_key, "%s.%s", node->prev->ctx_key, node->key);
    node->ctx_fld_off = node->prev->ctx_fld_off + node->fld.off;
    node->ctx_upd_off = node->prev->ctx_fld_off + node->upd.off;
  }

  ht_set(by_off, ht_key_int(node->ctx_fld_off), node);
  ht_set(by_key, ht_key_str(node->ctx_key),     node);

  if (!info->subs)
    return node;

  /* sort in reverse order for easy travers with dfs */
  qsort(info->subs, info->subs_n, sizeof *info->subs, (comparison_fn_t)(cfg_info_cmp_by_off));

  node->next_n = info->subs_n;
  node->next   = calloc(node->next_n, sizeof *node->next);
  for (i = 0; i < info->subs_n; ++i)
    node->next[i] = cfg_tree_node_reify(&info->subs[i], node, by_off, by_key);

  return node;
}

static bool
cfg_ctx_dump_node(cfg_node *node, bool bound, cfg_u32 depth, void *data)
{
  cfg_ctx *ctx = data;

  logi("'%s' (%p) (%p)", node->ctx_key,
    CFG_NODE_FLD_PTR(ctx->cfg.ptr, node, void),
    CFG_NODE_UPD_PTR(ctx->cfg.ptr, node));
  logi("  .fld.type: %u", node->fld.type);
  logi("  .fld.off:  %u", node->fld.off);
  logi("  .fld.size: %u", node->fld.size);
  logi("  .upd.id:   %u", node->upd.id);
  logi("  .upd.off:  %u", node->upd.off);
  logi("  .upd.size: %u", node->upd.size);

  return true;
}

static void
cfg_ctx_dump(cfg_ctx *ctx) {
  cfg_tree_dfs(&ctx->tree, cfg_ctx_dump_node, ctx);
}

cfg_ctx *
cfg_ctx_create(void     *data,
               cfg_u32   size,
               cfg_info *infos,
               cfg_u32   infos_n,
               cfg_u32   infos_all_est_n)
{
  cfg_ctx  *ctx;
  cfg_tree *tree;
  cfg_u32   i;

  if (!data
   || !size
   || !infos
   || !infos_n
  ) return NULL;

  ctx = calloc(1, sizeof *ctx);
  if (!ctx)
    return NULL;

  ctx->cfg.ptr  = data;
  ctx->cfg.size = size;

  tree = &ctx->tree;
  tree->by_off = ht_create(HT_KEY_INT, infos_all_est_n);
  tree->by_key = ht_create(HT_KEY_STR, infos_all_est_n);

  tree->root.next_n = infos_n;
  tree->root.next   = calloc(infos_n, sizeof *tree->root.next);

  qsort(infos, infos_n, sizeof *infos, (comparison_fn_t)(cfg_info_cmp_by_off));

  for (i = 0; i < infos_n; ++i)
    tree->root.next[i] = cfg_tree_node_reify(&infos[i], NULL, tree->by_off, tree->by_key);

  /* any of hashtables will have total count of nodes */
  tree->count = ht_length(tree->by_off);

  /* cfg_ctx_dump(ctx); */

  return ctx;
}

void
cfg_ctx_destroy(cfg_ctx *ctx)
{
  if (!ctx)
    return;

  free(ctx);
}

static cfg_node const *
cfg_ctx_get_node_by_key(cfg_ctx const *ctx, char const *key)
{
  return ht_get(ctx->tree.by_key, ht_key_str(key));
}

static cfg_node const *
cfg_ctx_get_node_by_ref(cfg_ctx const *ctx, void const *ref)
{
  cfg_node const *node;
  cfg_u32         ref_off;

  if (ref < ctx->cfg.ptr) {
    logi("by_ref: out of bounds: %p < %p", ref, ctx->cfg.ptr);
    return NULL;
  }


  ref_off = (cfg_u08 *)(ref) - (cfg_u08 *)(ctx->cfg.ptr);
  if (ref_off >= ctx->cfg.size) {
    logi("by_ref: out of bounds: %u >= %u", ref_off, ctx->cfg.size);
    return NULL;
  }

  node = ht_get(ctx->tree.by_off, ht_key_int(ref_off));
  if (!node) {
    logi("by_ref: hashtable failed");
    return NULL;
  }
  return node;
}

cfg_ret
cfg_ctx_bind_file(cfg_ctx *ctx, char const *path)
{
  strncpy(ctx->file.path, path, sizeof ctx->file.path);
  return CFG_RET_SUCCESS;
}

typedef struct {
  cfg_ctx *ctx;
  FILE    *stream;
} cfg_file_ctx;

#define PRItab        "%*s"
#define ARGtab(depth) ((depth) * 2), ""

bool
cfg_ctx_save_node(cfg_node *node, bool bound, cfg_u32 depth, void *data)
{
  cfg_file_ctx *file   = data;
  cfg_ctx      *ctx    = file->ctx;
  FILE         *stream = file->stream;

  #define CFG_FLD_SAVE(tid, fmt)                               \
    case CFG_FLD_TYPE_NAME(tid): {                             \
      fprintf(stream, PRItab".%s = " fmt ",\n",                \
        ARGtab(depth), node->key,                              \
        *CFG_NODE_FLD_PTR(ctx->cfg.ptr, node, CFG_TYPE(tid))); \
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
    CFG_FLD_SAVE(CFG_TID_str, "\"%s\"");

    case CFG_FLD_TYPE_NAME(CFG_TID_obj): {
      fprintf(stream, PRItab".%s = {\n", ARGtab(depth), node->key);
    } break;

    case CFG_FLD_TYPE_NAME(CFG_TID_ptr):
      /* ignore ptrs (runtime-only feature) */

    default: {
    } break;
  }

  /* possible only if node was a nested object resident */
  if (bound)
    fprintf(stream, PRItab"}\n", ARGtab(depth - 1));

  return true;
}

cfg_ret
cfg_ctx_save_file(cfg_ctx *ctx)
{
  cfg_file_ctx file;

  memset(&file, 0, sizeof file);

  if (ctx->file.path[0] == '\0')
    return CFG_RET_INVALID;

  file.ctx = ctx;
  file.stream = fopen(ctx->file.path, "w");
  if (!file.stream)
    return CFG_RET_FAILRUE;

  fseek(file.stream, 0, SEEK_SET);

  fprintf(file.stream, "%#08x {\n", ctx->cfg.size);
  cfg_tree_dfs(&ctx->tree, cfg_ctx_save_node, &file);
  fprintf(file.stream, "}\n");

  fclose(file.stream);

  return CFG_RET_SUCCESS;
}

static cfg_ret
cfg_ctx_file_line_proc(char *line, cfg_u32 count, char **pkey, char **pval)
{
  #define is_nul_char(c) ((c) == '\0')
  #define is_com_char(c) ((c) == '#')
  #define is_sok_char(c) ((c) == '_' || isalpha(c))
  #define is_kvp_char(c) ((c) == '_' || (c) == '.' || (c) == '-' || isalnum(c))
  #define is_sep_char(c) ((c) == '=')

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
  cfg_file_ctx file;
  char         line[LINE_MAX];
  cfg_u32      line_i;

  memset(&file, 0, sizeof file);
  memset(line, 0, sizeof line);
  line_i = 0;

  if (ctx->file.path[0] == '\0')
    return CFG_RET_INVALID;

  file.ctx = ctx;
  file.stream = fopen(ctx->file.path, "r");
  if (!file.stream)
    return CFG_RET_INVALID;

  fseek(file.stream, 0, SEEK_SET);

  while (++line_i, fgets(line, sizeof line - 1, file.stream)) {
    char *c = line;
  }

  fclose(file.stream);

  return CFG_RET_SUCCESS;
}

static cfg_ret
cfg_ctx_set_value_by_ref(cfg_ctx            *ctx,
                         void         const *ref,
                         void         const *pval,
                         cfg_fld_type        type)
{
  cfg_node const *node;
  cfg_ret         ret;

  if (!ctx || !ref)
    return CFG_RET_INVALID;

  node = cfg_ctx_get_node_by_ref(ctx, ref);
  if (!node)
    return CFG_RET_UNKNOWN;

  ret = cfg_node_req_check(node, type);
  if (ret != CFG_RET_SUCCESS)
    return ret;


  ret = cfg_node_set_value(ctx->cfg.ptr, node, pval);
  return ret;
}

static cfg_ret
cfg_ctx_set_value_by_key(cfg_ctx            *ctx,
                         char         const *key,
                         void         const *pval,
                         cfg_fld_type        type)
{
  cfg_node const *node;
  cfg_ret         ret;

  if (!ctx || !key)
    return CFG_RET_INVALID;

  node = cfg_ctx_get_node_by_key(ctx, key);
  if (!node)
    return CFG_RET_UNKNOWN;

  ret = cfg_node_req_check(node, type);
  if (ret != CFG_RET_SUCCESS)
    return ret;

  ret = cfg_node_set_value(ctx->cfg.ptr, node, pval);
  return ret;
}

static cfg_ret
cfg_ctx_get_value_by_key(cfg_ctx      const *ctx,
                         char         const *key,
                         void               *pval,
                         cfg_fld_type        type)
{
  cfg_node const *node;
  cfg_ret         ret;

  if (!ctx || !key)
    return CFG_RET_INVALID;

  node = cfg_ctx_get_node_by_key(ctx, key);
  if (!node)
    return CFG_RET_UNKNOWN;

  ret = cfg_node_req_check(node, type);
  if (ret != CFG_RET_SUCCESS)
    return ret;

  ret = cfg_node_get_value(ctx->cfg.ptr, node, pval);
  return ret;
}


#define DECLARE_CFG_CTX_SET_BY_REF(tid)                   \
  cfg_ret                                                 \
  CFG_CTX_SET_BY_REF_NAME(tid)(cfg_ctx             *ctx,  \
                               void          const *ref,  \
                               CFG_TYPE(tid) const  val)  \
  {                                                       \
    return cfg_ctx_set_value_by_ref(                      \
      ctx, ref, (void *)(&val), CFG_FLD_TYPE_NAME(tid));  \
  }

#define DECLARE_CFG_CTX_SET_BY_KEY(tid)                   \
  cfg_ret                                                 \
  CFG_CTX_SET_BY_KEY_NAME(tid)(cfg_ctx             *ctx,  \
                               char          const *key,  \
                               CFG_TYPE(tid) const  val)  \
  {                                                       \
    return cfg_ctx_set_value_by_key(                      \
      ctx, key, (void *)(&val), CFG_FLD_TYPE_NAME(tid));  \
  }

#define DECLARE_CFG_CTX_GET_BY_KEY(tid)                   \
  cfg_ret                                                 \
  CFG_CTX_GET_BY_KEY_NAME(tid)(cfg_ctx       const *ctx,  \
                               char          const *key,  \
                               CFG_TYPE(tid)       *val)  \
  {                                                       \
    return cfg_ctx_get_value_by_key(                      \
      ctx, key, (void *)(val), CFG_FLD_TYPE_NAME(tid));   \
  }


EXPAND_CFG_TIDS_ALL(DECLARE_CFG_CTX_SET_BY_REF)
EXPAND_CFG_TIDS_ALL(DECLARE_CFG_CTX_SET_BY_KEY)

EXPAND_CFG_TIDS_BASE(DECLARE_CFG_CTX_GET_BY_KEY)
EXPAND_CFG_TIDS_PTRS(DECLARE_CFG_CTX_GET_BY_KEY)


/* WARNING: getter for `CFG_TID_obj` is a special case
            as we do not want a pointer to pointer to object */
cfg_ret
CFG_CTX_GET_BY_KEY_NAME(CFG_TID_obj)(cfg_ctx       *ctx,
                                     char    const *key,
                                     cfg_obj        obj)
{
  return cfg_ctx_get_value_by_key(
    ctx, key, (void *)(obj), CFG_FLD_TYPE_NAME(CFG_TID_obj));
}