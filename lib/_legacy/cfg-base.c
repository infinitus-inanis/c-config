#define _GNU_SOURCE /* for asprintf */

#include "cfg-base.h"
#include "utils/hashtable.h"

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <stddef.h>
#include <stdio.h>
#include <regex.h>

#include <ctype.h>
#include <inttypes.h>

#define logi(fmt, args...) printf("[cfg]: "fmt "\n", ## args)

#define cfg_str2tid(tid) CONCATENATE(cfg_str2, tid)

static cfg_s64
cfg_str2sXX(char const *s, char **pend, cfg_s64 min, cfg_s64 max)
{
  cfg_s64 ret;
  errno = 0;
  ret = strtoll(s, pend, 10);
  if (ret < min) {
    errno = ERANGE;
    return min;
  }
  if (ret > max) {
    errno = ERANGE;
    return max;
  }
  return ret;
}
#define cfg_str2s08(s, pe) (CFG_TYPE(CFG_TID_s08)) cfg_str2sXX(s, pe, INT8_MIN,  INT8_MAX)
#define cfg_str2s16(s, pe) (CFG_TYPE(CFG_TID_s16)) cfg_str2sXX(s, pe, INT16_MIN, INT16_MAX)
#define cfg_str2s32(s, pe) (CFG_TYPE(CFG_TID_s32)) cfg_str2sXX(s, pe, INT32_MIN, INT32_MAX)
#define cfg_str2s64(s, pe) (CFG_TYPE(CFG_TID_s64)) cfg_str2sXX(s, pe, INT64_MIN, INT64_MAX)

static cfg_u64
cfg_str2uXX(char const *s, char **pend, cfg_u64 max) {
  cfg_u64 ret;
  errno = 0;
  if (*s == '-') {
    errno = EINVAL;
    return 0;
  }
  ret = strtoull(s, pend, 10);
  if (ret > max) {
    errno = ERANGE;
    return max;
  }
  return ret;
}
#define cfg_str2u08(s, pe) (CFG_TYPE(CFG_TID_u08)) cfg_str2uXX(s, pe, UINT8_MAX)
#define cfg_str2u16(s, pe) (CFG_TYPE(CFG_TID_u16)) cfg_str2uXX(s, pe, UINT16_MAX)
#define cfg_str2u32(s, pe) (CFG_TYPE(CFG_TID_u32)) cfg_str2uXX(s, pe, UINT32_MAX)
#define cfg_str2u64(s, pe) (CFG_TYPE(CFG_TID_u64)) cfg_str2uXX(s, pe, UINT64_MAX)

static inline cfg_f64
cfg_str2f64(char const *s, char **pend)
{
  errno = 0;
  return strtod(s, pend);
}

static inline cfg_f32
cfg_str2f32(char const *s, char **pend)
{
  errno = 0;
  return strtof(s, pend);
}

static int
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
  cfg_u32    nnext;
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
  cfg_u32   size;
  ht_ctx   *by_key;
  ht_ctx   *by_off;
} cfg_tree;

typedef bool (* cfg_tree_visit_f)(cfg_node *node, bool bound, cfg_u32 depth, void *data);

typedef struct {
  void    *data;
  bool     bound;
  cfg_u32  depth;
} tvs_node;

void
cfg_tree_dfs(cfg_tree *tree, cfg_tree_visit_f visit, void *data)
{
  tvs_node *stack, *stemp, scurr;
  cfg_s32   arrow;
  cfg_node *tnode;
  cfg_u32   i;

  if (tree->size == 0)
    return;

  tnode = &tree->root;
  if (!tnode->nnext)
    return;

  stack = calloc(tree->size, sizeof *stack);
  arrow = -1;

  stemp = &stack[++arrow];
  stemp->data  = tnode;
  stemp->bound = false;
  stemp->depth = 0;

  while (!(arrow < 0)) {
    scurr = stack[arrow--];
    tnode = scurr.data;
    if (!visit(tnode, scurr.bound, scurr.depth, data))
      break;

    /* NOTE (butsuk_d):
        traverse in reverse because nodes are
        sorted by offset in a natural order */

    if (!tnode->nnext)
      continue;

    for (i = tnode->nnext; i > 0; --i) {
      stemp = &stack[++arrow];
      stemp->data  = tnode->next[i - 1];
      stemp->bound = tnode->nnext == i;
      stemp->depth = scurr.depth + 1;
    }
  }

  free(stack);
}

struct cfg_ctx {
  cfg_ptr   data;
  cfg_tree  tree;
  cfg_file  file;
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

  if (!info->sub)
    return node;

  /* sort in reverse order for easy travers with dfs */
  qsort(info->sub, info->nsub, sizeof *info->sub, (comparison_fn_t)(cfg_info_cmp_by_off));

  node->nnext = info->nsub;
  node->next   = calloc(node->nnext, sizeof *node->next);
  for (i = 0; i < info->nsub; ++i)
    node->next[i] = cfg_tree_node_reify(&info->sub[i], node, by_off, by_key);

  return node;
}

static bool
cfg_ctx_dump_node(cfg_node *node, bool bound, cfg_u32 depth, void *data)
{
  cfg_ctx *ctx = data;

  (void)bound;
  (void)depth;

  logi("'%s' (%p) (%p)", node->ctx_key,
    CFG_NODE_FLD_PTR(ctx->data, node, void),
    CFG_NODE_UPD_PTR(ctx->data, node));
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
cfg_ctx_create(char     *type,
               cfg_u32   size,
               cfg_info *infos,
               cfg_u32   infos_n,
               cfg_u32   infos_all_est_n)
{
  cfg_ctx  *ctx;
  cfg_tree *tree;
  cfg_u32   i;

  if (!type
   || !size
   || !infos
   || !infos_n
  ) return NULL;

  ctx = calloc(1, sizeof *ctx);
  if (!ctx)
    return NULL;

  tree = &ctx->tree;
  tree->by_off = ht_create(HT_KEY_INT, infos_all_est_n);
  tree->by_key = ht_create(HT_KEY_STR, infos_all_est_n);

  /* config itself is a node that */
  tree->root.key      = strdup(type);
  tree->root.fld.type = CFG_FLD_TYPE_NAME(CFG_TID_obj);
  tree->root.fld.off  = 0;
  tree->root.fld.size = size;
  tree->root.nnext    = infos_n;
  tree->root.next     = calloc(infos_n, sizeof *tree->root.next);

  /* sorting nodes by offset in natural order is crutial for
     other parts of algorithm */
  qsort(infos, infos_n, sizeof *infos, (comparison_fn_t)(cfg_info_cmp_by_off));

  for (i = 0; i < infos_n; ++i)
    tree->root.next[i] = cfg_tree_node_reify(&infos[i], NULL, tree->by_off, tree->by_key);

  /* any of hashtables will have total count of nodes */
  tree->size = ht_length(tree->by_off);

  (void)cfg_ctx_dump;
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

cfg_ret
cfg_ctx_bind_data(cfg_ctx *ctx, void *data, cfg_u32 size)
{
  if (ctx->tree.root.fld.size != size)
    return CFG_RET_INVALID;

  ctx->data = data;
  return CFG_RET_SUCCESS;
}

static cfg_node const *
cfg_ctx_get_node_by_key(cfg_ctx const *ctx, char const *key)
{
  cfg_node const *node;

  node = ht_get(ctx->tree.by_key, ht_key_str(key));
  if (!node) {
    logi("by_key: no hashtable entry for: %s", key);
    return NULL;
  }

  return node;
}

static cfg_node const *
cfg_ctx_get_node_by_ref(cfg_ctx const *ctx, void const *ref)
{
  cfg_node const *node;
  cfg_u32         off;

  if (ref < ctx->data) {
    logi("by_ref: out of bounds: %p < %p", ref, ctx->data);
    return NULL;
  }

  off = (cfg_u08 *)(ref) - (cfg_u08 *)(ctx->data);
  if (off >= ctx->tree.root.fld.size) {
    logi("by_ref: out of bounds: %u >= %u",
      off, ctx->tree.root.fld.size);
    return NULL;
  }

  node = ht_get(ctx->tree.by_off, ht_key_int(off));
  if (!node) {
    logi("by_ref: no hashtable entry for: %u", off);
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

#define Indented(str)   "%*s" str
#define IndentArg(depth) ((depth) * 2), ""

static bool
cfg_ctx_save_node(cfg_node *node, bool bound, cfg_u32 depth, void *data)
{
  cfg_file_ctx *file   = data;
  cfg_ctx      *ctx    = file->ctx;
  FILE         *stream = file->stream;
  bool          isroot = node == &ctx->tree.root;

  #define CFG_FLD_SAVE(tid, fmt)                            \
    case CFG_FLD_TYPE_NAME(tid): {                          \
      fprintf(stream, Indented(".%s = " fmt ",\n"),         \
        IndentArg(depth), node->key,                        \
        *CFG_NODE_FLD_PTR(ctx->data, node, CFG_TYPE(tid))); \
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
      /* root node key constains structure type */
      if (isroot)
        fprintf(stream, Indented("(%s) {\n"), IndentArg(depth), node->key);
      else
        fprintf(stream, Indented(".%s = {\n"), IndentArg(depth), node->key);
    } break;

    case CFG_FLD_TYPE_NAME(CFG_TID_ptr):
      /* ignore ptrs (runtime-only feature) */

    default: {
    } break;
  }

  /* possible only if node was a nested object resident */
  if (bound)
    fprintf(stream, Indented("},\n"), IndentArg(depth - 1));

  return true;
}

cfg_ret
cfg_ctx_save_file(cfg_ctx *ctx)
{
  cfg_file_ctx file;
  cfg_u32      size;

  memset(&file, 0, sizeof file);

  if (ctx->file.path[0] == '\0')
    return CFG_RET_INVALID;

  file.ctx = ctx;
  file.stream = fopen(ctx->file.path, "w");
  if (!file.stream)
    return CFG_RET_FAILRUE;

  fseek(file.stream, 0, SEEK_SET);

  /* write config header that consists of structure size in hex */
  size = ctx->tree.root.fld.size;
  fprintf(file.stream, "0x%0*x\n", (int)(sizeof size * 2), size);

  /* write config fields node by node */
  cfg_tree_dfs(&ctx->tree, cfg_ctx_save_node, &file);

  fclose(file.stream);

  return CFG_RET_SUCCESS;
}

typedef struct {
  regex_t        reg;
  char    const *exp;
  int     const  cfl;
  size_t         nmatch;
} cfg_regex;

typedef enum {
  CFG_REGEX_TYPE_CFG_SIZE,
  CFG_REGEX_TYPE_CFG_TYPE,

  /* WARNING (butsuk_d):
     This regex doesn't cover errors in keys with multiple separators
     because POSIX doesn't support non-capturing groups. And capturing
     ones will broke matching logic.
     For example:
       `.fake.0000` - is NOT a valid key for C but WILL be matched.
       However it WILL be discarded during parsing of a config node
       because key for config node can't be constructed invalid by design. */
  CFG_REGEX_TYPE_CFG_FIELD
} cfg_regex_type;

static cfg_regex cfg_regex_tbl[] = {
  [CFG_REGEX_TYPE_CFG_SIZE] = {
    .exp = "\\s*0x([a-fA-F0-9]*)\\s*",
    .cfl = REG_EXTENDED | REG_NEWLINE,
    .nmatch = 2
  },
  [CFG_REGEX_TYPE_CFG_TYPE] = {
    .exp = "\\s*\\((.*)\\)\\s*\\{\\s*",
    .cfl = REG_EXTENDED | REG_NEWLINE,
    .nmatch = 2
  },
  [CFG_REGEX_TYPE_CFG_FIELD] = {
    .exp = "\\s*\\.([a-zA-Z_]+[\\.a-zA-Z0-9_]*)\\s*=\\s*(\".*\"|-?[0-9]+\\.?[0-9]*|\\{)\\s*\\,?\\s*",
    .cfl = REG_EXTENDED | REG_NEWLINE,
    .nmatch = 3
  }
};

/* Works almost like `strtok()`:
    1. if `str` is specified regexec executed and
       full match is returned or NULL on failure
    2. if `str` is NULL next match group is returned or NULL if none left
   Be aware that last char after match is replaced to '\0'
   and will be restored only on next call. */
static char *
cfg_regex_exec(cfg_regex_type type, char *str)
{
  static regmatch_t  __s_match[32] = {};
  static size_t      __s_idx = 0;
  static char       *__s_str = NULL;
  static char        __s_eos = '\0';

  regmatch_t *match;

  if (str) {
    cfg_regex *regex = &cfg_regex_tbl[type];
    int        ret = 0;
    char       err[256];
    size_t     i;

    for (i = 0; i < sizeof __s_match / sizeof 0[__s_match]; ++i) {
      __s_match[i].rm_so = -1;
      __s_match[i].rm_eo = -1;
    }

    __s_idx = 0;
    __s_str = str;
    __s_eos = '\0';

    ret = regexec(&regex->reg, __s_str, regex->nmatch, __s_match, 0);
    if (ret != REG_NOERROR) {
      regerror(ret, &regex->reg, err, sizeof err);
      logi("regex: failed with error: %s", err);
      return NULL;
    }
  } else {
    /* sanity check */
    if (__s_idx >= sizeof __s_match / sizeof 0[__s_match])
      return NULL;

    /* fix eol of previous match */
    match = &__s_match[__s_idx - 1];
    if (__s_eos)
      __s_str[match->rm_eo] = __s_eos;
  }

  match = &__s_match[__s_idx++];
  if (match->rm_so < 0 || match->rm_eo < 0)
    return NULL;

  __s_eos = __s_str[match->rm_eo];
  __s_str[match->rm_eo] = '\0';
  return &__s_str[match->rm_so];
}

static void cfg_regex_ctor() __attribute__((constructor));
static void cfg_regex_ctor() {
  cfg_regex *r;
  size_t     i;
  int        ret;

  for (i = 0; i < sizeof cfg_regex_tbl / sizeof 0[cfg_regex_tbl]; ++i) {
    r = &cfg_regex_tbl[i];
    ret = regcomp(&r->reg, r->exp, r->cfl);
    if (ret < 0)
      logi("regex[%zu]: failed to compile", i);
    // else
      // logi("regex[%zu]: compiled", i);
  }
}

static void cfg_regex_dtor() __attribute__((destructor));
static void cfg_regex_dtor() {
  cfg_regex *r;
  size_t     i;

  for (i = 0; i < sizeof cfg_regex_tbl / sizeof 0[cfg_regex_tbl]; ++i) {
    r = &cfg_regex_tbl[i];
    regfree(&r->reg);
    // logi("regex[%zu]: released", i);
  }
}

#define loadlogi(_f, _l, fmt, arg...) \
  logi("load[%s:%u] " fmt, _f, (cfg_u32)(_l), ## arg)

cfg_ret
cfg_ctx_load_file(cfg_ctx *ctx)
{
  char    *path;
  FILE    *stream;
  cfg_ret  ret = CFG_RET_INVALID;

  if (!ctx->tree.size) {
    logi("load: cfg tree isn't valid");
    return ret;
  }
  if (!ctx->file.path[0]) {
    logi("load: cfg file path isn't bound");
    return ret;
  }

  path = ctx->file.path;
  stream = fopen(path, "r");
  if (!stream) {
    logi("load: failed to open file at path: %s", path);
    return ret;
  }


  fseek(stream, 0, SEEK_SET);
  { /* parsing */
    char     rbuf[LINE_MAX] = {};
    cfg_u32  rlen = sizeof rbuf - 1;
    cfg_u32  line = 0;
    char    *str, *tmp;

    { /* header */
      cfg_u32 size;

      if (line++, !fgets(rbuf, rlen, stream))
        goto out;

      str = cfg_regex_exec(CFG_REGEX_TYPE_CFG_SIZE, rbuf);
      if (!str)
        goto out;
      str = cfg_regex_exec(0, NULL); /* get sub-match */

      size = strtoul(str, NULL, 16);
      if (size != ctx->tree.root.fld.size) {
        loadlogi(path, line, "cfg size doesn't match runtime value: %u != %u",
          size, ctx->tree.root.fld.size);
        goto out;
      }
      loadlogi(path, line, "cfg size: %u", size);

      memset(rbuf, 0, sizeof rbuf);
      if (line++, !fgets(rbuf, rlen, stream))
        goto out;

      str = cfg_regex_exec(CFG_REGEX_TYPE_CFG_TYPE, rbuf);
      if (!str)
        goto out;
      str = cfg_regex_exec(0, NULL); /* get sub-match */

      if (strcmp(str, ctx->tree.root.key)) {
        loadlogi(path, line, "cfg type doesn't match runtime value: %s != %s",
          str, ctx->tree.root.key);
        goto out;
      }
      loadlogi(path, line, "cfg type: %s", str);
    }
    { /* fields */
      cfg_node  *base;
      cfg_u32    i;

      base = &ctx->tree.root;
      while (line++, fgets(rbuf, rlen, stream) && base) {
        cfg_node *node;

        /* check for object group closing */
        str = strchr(rbuf, '}');
        if (str) {
          base = base->prev;
          continue;
        }

        /* check if line matches at all */
        str = cfg_regex_exec(CFG_REGEX_TYPE_CFG_FIELD, rbuf);
        if (!str) {
          loadlogi(path, line, "field regex failure");
          continue;
        }

        /* get first sub-match that is a field key */
        str = cfg_regex_exec(0, NULL);
        if (!str) {
          loadlogi(path, line, "line has no key");
          continue;
        }

        /* search for node by key */
        for (i = 0; i < base->nnext; ++i) {
          node = base->next[i];
          if (!strcmp(node->key, str))
            break;
        }
        if (i == base->nnext) {
          loadlogi(path, line, "unknown key: %s", str);
          continue;
        }

        /* get second sub-match that is a field value */
        str = cfg_regex_exec(0, NULL);
        if (!str) {
          loadlogi(path, line, "line has no value");
          continue;
        }

        #define CFG_FLD_LOAD(tid)                                       \
          case CFG_FLD_TYPE_NAME(tid): {                                \
            CFG_TYPE(tid) val = cfg_str2tid(tid)(str, &tmp);            \
            cfg_ret       set;                                          \
            if (errno == EINVAL || !tmp || *tmp || tmp == str) {        \
              loadlogi(path, line,                                      \
                "value (%s) for key (%s) of type (%s) is invalid",      \
                str, node->key, STRINGIFY(CFG_TYPE(tid)));              \
              break;                                                    \
            }                                                           \
            if (errno == ERANGE) {                                      \
              loadlogi(path, line,                                      \
                "value (%s) for key (%s) of type (%s) is out of range", \
                str, node->key, STRINGIFY(CFG_TYPE(tid)));              \
              break;                                                    \
            }                                                           \
            set = cfg_node_set_value(ctx->data, node, (void *)(&val));  \
            if (set != CFG_RET_SUCCESS) {                               \
              loadlogi(path, line,                                      \
                "value (%s) for key (%s) of type (%s) was not set",     \
                str, node->key, STRINGIFY(CFG_TYPE(tid)));              \
              break;                                                    \
            }                                                           \
          } break;

        switch (node->fld.type) {
          EXPAND_CFG_TIDS_BASE(CFG_FLD_LOAD);

          case CFG_FLD_TYPE_NAME(CFG_TID_str): {
            cfg_str val;
            cfg_ret set;
            val = strchr(str,   '\"');
            tmp = strchr(++val, '\"');
            if (!val || !tmp) {
              loadlogi(path, line,
                "value (%s) for key (%s) of type (%s) is invalid",
                str, node->key, STRINGIFY(cfg_str));
              break;
            }
            *tmp = '\0';
            set = cfg_node_set_value(ctx->data, node, (void *)(&val));
            if (set != CFG_RET_SUCCESS) {
              loadlogi(path, line,
                "value (%s) for key (%s) of type (%s) was not set",
                str, node->key, STRINGIFY(cfg_str));
              break;
            }
          } break;

          case CFG_FLD_TYPE_NAME(CFG_TID_obj): {
            base = node;
          } break;

          default: {
          } break;
        }
      }
    }
  }
  ret = CFG_RET_SUCCESS;

out:
  fclose(stream);
  return ret;
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

  ret = cfg_node_set_value(ctx->data, node, pval);
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

  ret = cfg_node_set_value(ctx->data, node, pval);
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

  ret = cfg_node_get_value(ctx->data, node, pval);
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
