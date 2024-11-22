#define _GNU_SOURCE /* strdup, qsort, etc.*/

#include "xcfg-tree.h"
#include "utils/traverse.h"
#include "utils/hashtable.h"

#include <unistd.h>
#include <stdlib.h>
#include <memory.h>

#include <stdio.h>
#define logi(fmt, args...) printf("[xcfg-tree]: "fmt "\n", ## args)

static int
xcfg_fld_cmp_by_off(xcfg_fld *l, xcfg_fld *r) {
  if (l->ref.off < r->ref.off)
    return -1;
  if (l->ref.off > r->ref.off)
    return  1;
  return 0;
}

void
xcfg_node_destroy(xcfg_node *node)
{
  if (!node)
    return;

  free(node->data_key);
  free(node->key);
  free(node);
}

#define xcfg_data_off(data, off, type) \
  (type)((xcfg_u08 *)(data) + (xcfg_u32)(off))

xcfg_ptr
xcfg_node_data_ref_ptr(xcfg_node *node, xcfg_ptr data)
{
  return xcfg_data_off(data, node->data_ref_off, xcfg_ptr);
}

static xcfg_upd *
xcfg_node_data_upd_ptr(xcfg_node *node, xcfg_ptr data)
{
  return xcfg_data_off(data, node->data_upd_off, xcfg_upd *);
}

xcfg_ret
xcfg_node_type_check(xcfg_node *node, xcfg_fld_type type)
{
  xcfg_fld_type ntype = node->type;
  xcfg_u32      nsize = node->ref.size;

  /* basic value types of the same size can accept
      any value of that size */
  switch (type) {
    case XCFG_FLD_TYPE_s08:
    case XCFG_FLD_TYPE_u08: {
      if (sizeof(xcfg_s08) != nsize)
        return XCFG_RET_INVALID;
    } break;

    case XCFG_FLD_TYPE_s16:
    case XCFG_FLD_TYPE_u16: {
      if (sizeof(xcfg_s16) != nsize)
        return XCFG_RET_INVALID;
    } break;

    case XCFG_FLD_TYPE_s32:
    case XCFG_FLD_TYPE_u32: {
      if (sizeof(xcfg_s32) != nsize)
        return XCFG_RET_INVALID;
    } break;

    case XCFG_FLD_TYPE_s64:
    case XCFG_FLD_TYPE_u64: {
      if (sizeof(xcfg_u64) != nsize)
        return XCFG_RET_INVALID;
    } break;

    default: {
      if (type != ntype)
        return XCFG_RET_INVALID;
    } break;
  }

  return XCFG_RET_SUCCESS;
}

void
xcfg_node_clear_upd(xcfg_node *node, xcfg_ptr data)
{
  *xcfg_node_data_upd_ptr(node, data) &= ~XCFG_UPD(node->upd.id);
}

void
xcfg_node_raise_upd(xcfg_node *node, xcfg_ptr data)
{
  do {
    *xcfg_node_data_upd_ptr(node, data) |= XCFG_UPD(node->upd.id);
  } while ((node = node->prev));
}

xcfg_ret
xcfg_node_set_value(xcfg_node *node, xcfg_ptr data, xcfg_ptr pval)
{
  xcfg_ptr dst = xcfg_node_data_ref_ptr(node, data);
  xcfg_ptr src;
  bool     cmp;

  #define XCFG_SFX_DO_EXPAND(sfx) case XCFG_FLD_TYPE(sfx):
  switch (node->type) {
    XCFG_SFX_EXPAND_val() {
      src = pval;
      cmp = true;
    } break;

    XCFG_SFX_EXPAND_ptr() {
      src = pval;
      cmp = false;
    } break;

    XCFG_SFX_EXPAND_obj() {
      src = *(xcfg_obj *)(pval);
      cmp = true;
    } break;

    XCFG_SFX_EXPAND_str() {
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

xcfg_ret
xcfg_node_get_value(xcfg_node *node, xcfg_ptr data, xcfg_ptr pval)
{
  xcfg_ptr src = xcfg_node_data_ref_ptr(node, data);
  xcfg_ptr dst;

  #define XCFG_SFX_DO_EXPAND(sfx) case XCFG_FLD_TYPE(sfx):
  switch (node->type) {
    XCFG_SFX_EXPAND_val()
    XCFG_SFX_EXPAND_ptr()
    XCFG_SFX_EXPAND_obj() {
      dst = pval;
    } break;

    XCFG_SFX_EXPAND_str() {
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

typedef struct {
  xcfg_node_tvs_visit_f visit;
  xcfg_ptr              context;
} xcfg_tree_tvs;

static bool
xcfg_node_tvs_visit(void *context, tvs_node *curr)
{
  xcfg_tree_tvs *ttvs = context;
  xcfg_node_tvs  ntvs = {
    .node = curr->data,
    .depth = curr->depth,
    .meta.bound_0 = curr->meta.next_0,
    .meta.bound_N = curr->meta.next_N
  };
  return ttvs->visit(&ntvs, ttvs->context) == XCFG_RET_SUCCESS;
}

static bool
xcfg_node_tvs_populate(void *context, tvs_node *curr, void ***pnext, uint32_t *nnext)
{
  (void)context;

  xcfg_node *node = curr->data;
  if (!node)
    return false;

  if (!node->nnext)
    return false;

  *pnext = (void **) (node->next);
  *nnext = (uint32_t)(node->nnext);
  return true;
}

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

  /* sorting nodes by offset in natural order
      is crutial for other parts of this system */
  qsort(fld->sub, fld->nsub, sizeof *fld->sub,
    (comparison_fn_t)(xcfg_fld_cmp_by_off));

  node->nnext = fld->nsub;
  node->next  = calloc(node->nnext, sizeof *node->next);
  for (i = 0; i < fld->nsub; ++i)
    node->next[i] = xcfg_tree_create_node(&fld->sub[i], node, by_off, by_key);

  return node;
}

xcfg_ret
xcfg_tree_build(xcfg_tree *tree,
                xcfg_str   root_key,
                xcfg_u32   root_size,
                xcfg_fld  *root_pfld,
                xcfg_u32   root_nfld)
{
  xcfg_node *root = &tree->root;
  xcfg_u32   i;

  if (!root_key  || !root_size
   || !root_pfld || !root_nfld
  ) return XCFG_RET_INVALID;

  tree->by_off = ht_create(HT_KEY_INT, 32);
  tree->by_key = ht_create(HT_KEY_STR, 32);

  root->key      = strdup(root_key);
  root->type     = XCFG_FLD_TYPE(XCFG_SFX_obj);
  root->ref.off  = 0;
  root->ref.size = root_size;
  root->nnext    = root_nfld;
  root->next     = calloc(root->nnext, sizeof *root->next);

  /* sorting nodes by offset in natural order
      is crutial for other parts of this system */
  qsort(root_pfld, root_nfld, sizeof *root_pfld,
    (comparison_fn_t)(xcfg_fld_cmp_by_off));

  for (i = 0; i < root_nfld; ++i) {
    root->next[i] = xcfg_tree_create_node(&root_pfld[i], NULL, tree->by_off, tree->by_key);
    root->next[i]->prev = &tree->root;
  }

  /* any of the hashtables will have total count of nodes */
  tree->size = ht_length(tree->by_off);

  return XCFG_RET_SUCCESS;
}

void
xcfg_tree_tvs_depth_first(xcfg_tree *tree, xcfg_node_tvs_visit_f visit, xcfg_ptr context)
{
  xcfg_tree_tvs ttvs = {
    .visit   = visit,
    .context = context
  };
  tvs_depth_first(&ttvs,
    (void **)  (tree->root.next),
    (uint32_t) (tree->root.nnext),
    (uint32_t) (tree->size),
    xcfg_node_tvs_visit,
    xcfg_node_tvs_populate);
}

static xcfg_ret
xcfg_node_tvs_do_destroy(xcfg_node_tvs *curr, xcfg_ptr context)
{
  (void)context;

  xcfg_node *node = curr->node;
  if (!node)
    return XCFG_RET_INVALID;

  if (curr->meta.bound_N)
    free(node->prev->next);

  xcfg_node_destroy(node);
  return XCFG_RET_SUCCESS;
}

void
xcfg_tree_dispose(xcfg_tree *tree)
{
  xcfg_tree_tvs_depth_first(tree, xcfg_node_tvs_do_destroy, NULL);
  free(tree->root.key);
}

static xcfg_ret
xcfg_node_tvs_do_dump(xcfg_node_tvs *curr, void *data)
{
  xcfg_node *node = curr->node;
  if (!node)
    return XCFG_RET_INVALID;

  logi(INDENT(".%s (%p)"), INDENTARG(curr->depth),
    node->key, xcfg_node_data_ref_ptr(node, data));

  return XCFG_RET_SUCCESS;
}

void
xcfg_tree_dump(xcfg_tree *tree, xcfg_ptr data)
{
  xcfg_tree_tvs_depth_first(tree, xcfg_node_tvs_do_dump, data);
}

xcfg_node *
xcfg_tree_get_node_by_off(xcfg_tree *tree, xcfg_u32 off)
{
  return (xcfg_node *)(ht_get(tree->by_off, ht_key_int(off)));
}

xcfg_node *
xcfg_tree_get_node_by_key(xcfg_tree *tree, xcfg_str key)
{
  return (xcfg_node *)(ht_get(tree->by_key, ht_key_str(key)));
}