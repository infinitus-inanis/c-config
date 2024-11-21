#define _GNU_SOURCE /* strdup, qsort, etc.*/

#include "xcfg-api.h"
#include "xcfg-fld.h"

#include "utils/hashtable.h"
#include "utils/traverse.h"

#include <unistd.h>
#include <stdlib.h>
#include <memory.h>

#include <stdio.h>
#define logi(fmt, args...) printf("[cfg]: "fmt "\n", ## args)

static int
xcfg_fld_cmp_by_off(xcfg_fld *l, xcfg_fld *r) {
  if (l->ref.off < r->ref.off)
    return -1;
  if (l->ref.off > r->ref.off)
    return  1;
  return 0;
}

typedef struct xcfg_node xcfg_node;
struct xcfg_node {
  xcfg_str data_key;
  xcfg_u32 data_ref_off;
  xcfg_u32 data_upd_off;

  xcfg_str      key;
  xcfg_ref_desc ref;
  xcfg_fld_type type;

  xcfg_upd_desc upd;

  xcfg_node  *prev;
  xcfg_node **next;
  xcfg_u32    nnext;
};

#define xcfg_data_off(data, off, type) \
  (type)((xcfg_u08 *)(data) + (xcfg_u32)(off))

#define xcfg_node_data_ref_off(node, data, type) \
  xcfg_data_off(data, (node)->data_ref_off, type)

#define xcfg_node_data_upd_ptr(node, data) \
  xcfg_data_off(data, (node)->data_upd_off, xcfg_upd *)

static xcfg_ret
xcfg_node_type_check(xcfg_node const *node, xcfg_fld_type const type)
{
  xcfg_fld_type const fld_type = node->type;
  xcfg_u32      const fld_size = node->ref.size;

  /* basic value types of the same size can accept
      any value of that size */
  switch (type) {
    case XCFG_FLD_TYPE_s08:
    case XCFG_FLD_TYPE_u08: {
      if (sizeof(xcfg_s08) != fld_size)
        return XCFG_RET_INVALID;
    } break;

    case XCFG_FLD_TYPE_s16:
    case XCFG_FLD_TYPE_u16: {
      if (sizeof(xcfg_s16) != fld_size)
        return XCFG_RET_INVALID;
    } break;

    case XCFG_FLD_TYPE_s32:
    case XCFG_FLD_TYPE_u32: {
      if (sizeof(xcfg_s32) != fld_size)
        return XCFG_RET_INVALID;
    } break;

    case XCFG_FLD_TYPE_s64:
    case XCFG_FLD_TYPE_u64: {
      if (sizeof(xcfg_u64) != fld_size)
        return XCFG_RET_INVALID;
    } break;

    default: {
      if (type != fld_type)
        return XCFG_RET_INVALID;
    } break;
  }

  return XCFG_RET_SUCCESS;
}

inline static void
xcfg_node_clear_upd(xcfg_node const *node, xcfg_ptr data)
{
  *xcfg_node_data_upd_ptr(node, data) &= ~XCFG_UPD(node->upd.id);
}

inline static void
xcfg_node_raise_upd(xcfg_node const *node, xcfg_ptr data)
{
  do {
    *xcfg_node_data_upd_ptr(node, data) |= XCFG_UPD(node->upd.id);
  } while ((node = node->prev));
}

static xcfg_ret
xcfg_node_set_value(xcfg_node const *node, xcfg_ptr data, xcfg_ptr pval)
{
  xcfg_ptr dst = xcfg_node_data_ref_off(node, data, xcfg_ptr);
  xcfg_ptr src;
  bool     cmp;

  #define XCFG_SFX_DO_EXPAND(sfx) case XCFG_FLD_TYPE(sfx):
  switch (node->type) {
    XCFG_SFX_EXPAND_VAL() {
      src = pval;
      cmp = true;
    } break;

    XCFG_SFX_EXPAND_PTR() {
      src = pval;
      cmp = false;
    } break;

    XCFG_SFX_EXPAND_OBJ() {
      src = *(xcfg_obj *)(pval);
      cmp = true;
    } break;

    XCFG_SFX_EXPAND_STR() {
      src = pval;
      xcfg_str src_str = *(xcfg_str *)(src);
      xcfg_str dst_str = *(xcfg_str *)(dst);
      xcfg_u32 src_sz = src_str ? strlen(src_str) + 1 : 0;
      xcfg_u32 dst_sz = dst_str ? strlen(dst_str) + 1 : 0;

      if (dst_sz == src_sz) {
        /* same size */
        if (dst_sz == 0)
          /* nothing to do for NULL strings */
          return XCFG_RET_SUCCESS;

        if (!memcmp(dst_str, src_str, dst_sz))
          /* nothing to do for equal strings */
          return XCFG_RET_SUCCESS;

      } else {
        /* diff size */
        if (src_str == 0) {
          /* setting NULL string */
          free(dst_str);
          return XCFG_RET_SUCCESS;
        }

        /* setup new address for string */
        dst_sz = src_sz;
        dst_str = realloc(dst_str, dst_sz);
        memcpy(dst, &dst_str, node->ref.size);
      }

      /* set new value */
      memcpy(dst_str, src_str, dst_sz);
      xcfg_node_raise_upd(node, data);
      return XCFG_RET_SUCCESS;
    } break;

    default:
      return XCFG_RET_INVALID;
  }
  #undef XCFG_SFX_DO_EXPAND

  if (cmp && !memcmp(dst, src, node->ref.size))
    return XCFG_RET_SUCCESS;

  memcpy(dst, src, node->ref.size);
  xcfg_node_raise_upd(node, data);

  return XCFG_RET_SUCCESS;
}

static xcfg_ret
xcfg_node_get_value(xcfg_node const *node, xcfg_ptr data, xcfg_ptr pval)
{
  xcfg_ptr src = xcfg_node_data_ref_off(node, data, xcfg_ptr);
  xcfg_ptr dst;

  #define XCFG_SFX_DO_EXPAND(sfx) case XCFG_FLD_TYPE(sfx):
  switch (node->type) {
    XCFG_SFX_EXPAND_VAL()
    XCFG_SFX_EXPAND_PTR()
    XCFG_SFX_EXPAND_OBJ() {
      dst = pval;
    } break;

    XCFG_SFX_EXPAND_STR() {
      dst = pval;
      xcfg_str src_str = *(xcfg_str *)(src);
      xcfg_str dst_str = *(xcfg_str *)(dst);
      xcfg_u32 src_sz  = src_str ? strlen(src_str) + 1 : 0;

      if (src_sz == 0)
        /* nothing to return */
        return XCFG_RET_SUCCESS;

      /* setup dst pointer */
      dst_str = realloc(dst_str, src_sz);
      memcpy(dst, &dst_str, node->ref.size);

      /* set value to dst */
      memcpy(dst_str, src_str, src_sz);
      return XCFG_RET_SUCCESS;
    }
    default:
      return XCFG_RET_INVALID;
  }
  #undef XCFG_SFX_DO_EXPAND

  memcpy(dst, src, node->ref.size);

  return XCFG_RET_SUCCESS;
}

bool
xcfg_node_tvs_populate(void *ctx, tvs_node *curr, void ***pnext, uint32_t *nnext)
{
  (void)ctx;

  xcfg_node *node = curr->data;
  if (!node)
    return false;

  if (!node->nnext)
    return false;

  *pnext = (void **) (node->next);
  *nnext = (uint32_t)(node->nnext);
  return true;
}

bool
xcfg_node_tvs_free(void *ctx, tvs_node *curr)
{
  (void)ctx;

  xcfg_node *node = curr->data;
  if (!node)
    return false;

  free(node->data_key);
  free(node->key);

  if (curr->meta.next_N)
    free(node->prev->next);

  return true;
}

bool
xcfg_node_tvs_dump(void *data, tvs_node *curr)
{
  xcfg_node *node = curr->data;
  if (!node)
    return false;

  logi(INDENT(".%s (%p)"), INDENTARG(curr->depth),
    node->key, xcfg_node_data_ref_off(node, data, xcfg_ptr));

  return true;
}


typedef struct {
  xcfg_node root;
  xcfg_u32  size;
  ht_ctx   *by_key;
  ht_ctx   *by_off;
} xcfg_tree;

static xcfg_node *
xcfg_tree_create_node(xcfg_fld *fld, xcfg_node *prev, ht_ctx *by_off, ht_ctx *by_key)
{
  xcfg_node *node = calloc(1, sizeof *node);
  xcfg_u32   i;

  node->key  = strdup(fld->key);
  node->type = fld->type;
  node->ref  = fld->ref;
  node->upd  = fld->upd;

  node->data_ref_off = node->ref.off;
  node->data_upd_off = node->upd.ref.off;

  node->prev = prev;
  if (!node->prev) {
    /* branch start */
    asprintf(&node->data_key, "%s", node->key);
  } else {
    /* branch child */
    asprintf(&node->data_key, "%s.%s", node->prev->data_key, node->key);
    node->data_ref_off += node->prev->data_ref_off;
    node->data_upd_off += node->prev->data_ref_off;
  }

  ht_set(by_off, ht_key_int(node->data_ref_off), node);
  ht_set(by_key, ht_key_str(node->data_key),     node);

  if (!fld->sub)
    return node;

  /* sort in reverse order for easy travers with dfs */
  qsort(fld->sub, fld->nsub, sizeof *fld->sub,
    (comparison_fn_t)(xcfg_fld_cmp_by_off));

  node->nnext = fld->nsub;
  node->next  = calloc(node->nnext, sizeof *node->next);
  for (i = 0; i < fld->nsub; ++i)
    node->next[i] = xcfg_tree_create_node(&fld->sub[i], node, by_off, by_key);

  return node;
}

struct xcfg {
  struct {
    xcfg_str name;
    xcfg_ptr data;
    xcfg_u32 size;
  } type;

  xcfg_tree tree;
};

static void
xcfg_dump(xcfg *ctx)
{
  logi("[xcfg]");
  logi("  type.name: %s", ctx->type.name);
  logi("  type.data: %p", ctx->type.data);
  logi("  type.size: %u", ctx->type.size);
  logi("[xcfg.tree]");
  tvs_depth_first(ctx->type.data,
    (void **)  (ctx->tree.root.next),
    (uint32_t) (ctx->tree.root.nnext),
    (uint32_t) (ctx->tree.size),
    xcfg_node_tvs_dump,
    xcfg_node_tvs_populate);
}

xcfg *
xcfg_create(xcfg_str  type_name,
            xcfg_u32  type_size,
            xcfg_fld *root_pfld,
            xcfg_u32  root_nfld)
{
  xcfg      *ctx;
  xcfg_tree *tree;
  xcfg_u32   i;

  if (!type_name || !type_size
   || !root_pfld || !root_nfld
  ) return NULL;

  ctx = calloc(1, sizeof *ctx);
  if (!ctx)
    return NULL;

  ctx->type.name = type_name;
  ctx->type.size = type_size;

  tree = &ctx->tree;
  tree->by_off = ht_create(HT_KEY_INT, 32);
  tree->by_key = ht_create(HT_KEY_STR, 32);

  /* config itself is a node that */
  tree->root.key      = strdup(ctx->type.name);
  tree->root.type     = XCFG_FLD_TYPE(XCFG_SFX_obj);
  tree->root.ref.off  = 0;
  tree->root.ref.size = ctx->type.size;
  tree->root.nnext    = root_nfld;
  tree->root.next     = calloc(tree->root.nnext, sizeof *tree->root.next);

  /* sorting nodes by offset in natural order
      is crutial for other parts of this system */
  qsort(root_pfld, root_nfld, sizeof *root_pfld,
    (comparison_fn_t)(xcfg_fld_cmp_by_off));

  for (i = 0; i < root_nfld; ++i) {
    tree->root.next[i] = xcfg_tree_create_node(&root_pfld[i], NULL, tree->by_off, tree->by_key);
    tree->root.next[i]->prev = &tree->root;
  }

  /* any of hashtables will have total count of nodes */
  tree->size = ht_length(tree->by_off);

  xcfg_dump(ctx);

  return ctx;
}

void
xcfg_destroy(xcfg *ctx)
{
  if (!ctx)
    return;

  tvs_depth_first(NULL,
    (void **)(ctx->tree.root.next),
    (size_t) (ctx->tree.root.nnext),
    (size_t) (ctx->tree.size),
    xcfg_node_tvs_free,
    xcfg_node_tvs_populate);

  free(ctx->tree.root.key);
  free(ctx);
}

xcfg_ret
xcfg_set_data(xcfg *ctx, xcfg_ptr data, xcfg_u32 size)
{
  if (ctx->type.size != size)
    return XCFG_RET_INVALID;

  ctx->type.data = data;
  return XCFG_RET_SUCCESS;
}

xcfg_ptr
xcfg_get_data(xcfg *ctx)
{
  return ctx->type.data;
}

xcfg_ret
xcfg_bind_file(xcfg *ctx, xcfg_str path)
{
  (void)ctx;
  (void)path;
  return XCFG_RET_SUCCESS;
}

xcfg_ret
xcfg_save_file(xcfg *ctx)
{
  (void)ctx;
  return XCFG_RET_SUCCESS;
}

xcfg_ret
xcfg_load_file(xcfg *ctx)
{
  (void)ctx;
  return XCFG_RET_SUCCESS;
}

static xcfg_node *
xcfg_get_node_by_ref(xcfg *ctx, xcfg_ptr ref)
{
  xcfg_u32 off;

  if (ref < ctx->type.data)
    return NULL;

  off = (xcfg_u08 *)(ref) - (xcfg_u08 *)(ctx->type.data);
  if (off >= ctx->type.size)
    return NULL;

  return (xcfg_node *)(ht_get(ctx->tree.by_off, ht_key_int(off)));
}

static xcfg_node *
xcfg_get_node_by_key(xcfg *ctx, xcfg_str key)
{
  return (xcfg_node *)(ht_get(ctx->tree.by_key, ht_key_str(key)));
}

static xcfg_ret
xcfg_set_by_ref_impl(xcfg *ctx, xcfg_ptr ref, xcfg_fld_type type, xcfg_ptr pval)
{
  xcfg_node *node;
  xcfg_ret   ret;

  if (!ctx || !ref || !type)
    return XCFG_RET_INVALID;

  node = xcfg_get_node_by_ref(ctx, ref);
  if (!node)
    return XCFG_RET_UNKNOWN;

  ret = xcfg_node_type_check(node, type);
  if (ret < 0)
    return ret;

  return xcfg_node_set_value(node, ctx->type.data, pval);
}

static xcfg_ret
xcfg_set_by_key_impl(xcfg *ctx, xcfg_str key, xcfg_fld_type type, xcfg_ptr pval)
{
  xcfg_node *node;
  xcfg_ret   ret;

  if (!ctx || !key || !type)
    return XCFG_RET_INVALID;

  node = xcfg_get_node_by_key(ctx, key);
  if (!node)
    return XCFG_RET_UNKNOWN;

  ret = xcfg_node_type_check(node, type);
  if (ret < 0)
    return ret;

  return xcfg_node_set_value(node, ctx->type.data, pval);
}

static xcfg_ret
xcfg_get_by_key_impl(xcfg *ctx, xcfg_str key, xcfg_fld_type type, xcfg_ptr pval)
{
  xcfg_node *node;
  xcfg_ret   ret;

  if (!ctx || !key || !type)
    return XCFG_RET_INVALID;

  node = xcfg_get_node_by_key(ctx, key);
  if (!node)
    return XCFG_RET_UNKNOWN;

  ret = xcfg_node_type_check(node, type);
  if (ret < 0)
    return ret;

  return xcfg_node_get_value(node, ctx->type.data, pval);
}

#define XCFG_SFX_DO_EXPAND(sfx) \
  xcfg_ret \
  XCFG_SET_BY_REF(sfx)(xcfg *ctx, xcfg_ptr ref, XCFG_TYPE(sfx) val) \
  { return xcfg_set_by_ref_impl(ctx, ref, XCFG_FLD_TYPE(sfx), (xcfg_ptr)(&val)); }

XCFG_SFX_EXPAND_ALL()
#undef XCFG_SFX_DO_EXPAND


#define XCFG_SFX_DO_EXPAND(sfx) \
  xcfg_ret \
  XCFG_SET_BY_KEY(sfx)(xcfg *ctx, xcfg_str key, XCFG_TYPE(sfx) val) \
  { return xcfg_set_by_key_impl(ctx, key, XCFG_FLD_TYPE(sfx), (xcfg_ptr)(&val)); }

XCFG_SFX_EXPAND_ALL()
#undef XCFG_SFX_DO_EXPAND


#define XCFG_SFX_DO_EXPAND(sfx) \
  xcfg_ret \
  XCFG_GET_BY_KEY(sfx)(xcfg *ctx, xcfg_str key, XCFG_TYPE(sfx) *pval) \
  { return xcfg_get_by_key_impl(ctx, key, XCFG_FLD_TYPE(sfx), (xcfg_ptr)(pval)); }

XCFG_SFX_EXPAND_VAL()
XCFG_SFX_EXPAND_PTR()
XCFG_SFX_EXPAND_STR()
#undef XCFG_SFX_DO_EXPAND


xcfg_ret
XCFG_GET_BY_KEY(XCFG_SFX_obj)(xcfg *ctx, xcfg_str key, xcfg_obj obj)
{ return xcfg_get_by_key_impl(ctx, key, XCFG_FLD_TYPE(XCFG_SFX_obj), (xcfg_ptr)(obj)); }