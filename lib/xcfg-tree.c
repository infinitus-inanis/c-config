#define _GNU_SOURCE /* strdup, qsort, etc.*/

#include "xcfg-tree.h"
#include "utils/traverse.h"
#include "utils/hashtable.h"

#include <unistd.h>
#include <stdlib.h>
#include <memory.h>

#include <stdio.h>
#define logi(fmt, args...) printf("[xcfg-tree]: "fmt "\n", ## args)

void
xcfg_node_destroy(xcfg_node *node)
{
  xcfg_u32 i;

  if (!node)
    return;

  for (i = 0; i < node->nnext; ++i)
    xcfg_node_destroy(node->next[i]);

  free(node->next);

  free(node->data_fld_key);
  free(node);
}

#define xcfg_data_off(data, off, type) \
  (type)((xcfg_u08 *)(data) + (xcfg_u32)(off))

xcfg_ptr
xcfg_node_get_fld_ptr(xcfg_node *node, xcfg_ptr data)
{
  return GET_FIELD(data, node->data_fld_off, xcfg_ptr);
}

static xcfg_upd *
xcfg_node_get_upd_ptr(xcfg_node *node, xcfg_ptr data)
{
  return GET_FIELD(data, node->data_upd_off, xcfg_upd *);
}

xcfg_ret
xcfg_node_type_check(xcfg_node *node, xcfg_tid tid)
{
  xcfg_tid ntid  = node->rtfi->tid;
  xcfg_u32 nsize = node->rtfi->fld.size;

  /* basic value types of the same size can accept
      any value of that size */
  switch (tid) {
    case XCFG_TID_s08:
    case XCFG_TID_u08: {
      if (sizeof(xcfg_s08) != nsize)
        return XCFG_RET_INVALID;
    } break;

    case XCFG_TID_s16:
    case XCFG_TID_u16: {
      if (sizeof(xcfg_s16) != nsize)
        return XCFG_RET_INVALID;
    } break;

    case XCFG_TID_s32:
    case XCFG_TID_u32: {
      if (sizeof(xcfg_s32) != nsize)
        return XCFG_RET_INVALID;
    } break;

    case XCFG_TID_s64:
    case XCFG_TID_u64: {
      if (sizeof(xcfg_u64) != nsize)
        return XCFG_RET_INVALID;
    } break;

    default: {
      if (tid != ntid)
        return XCFG_RET_INVALID;
    } break;
  }

  return XCFG_RET_SUCCESS;
}

void
xcfg_node_clear_upd(xcfg_node *node, xcfg_ptr data)
{
  *xcfg_node_get_upd_ptr(node, data) &= ~XCFG_UPD(node->rtfi->upd);
}

void
xcfg_node_raise_upd(xcfg_node *node, xcfg_ptr data)
{
  do {
    *xcfg_node_get_upd_ptr(node, data) |= XCFG_UPD(node->rtfi->upd);
  } while ((node = node->prev));
}

xcfg_ret
xcfg_node_set_value(xcfg_node *node, xcfg_ptr data, xcfg_ptr pval)
{
  xcfg_ptr dst = xcfg_node_get_fld_ptr(node, data);
  xcfg_ptr src;
  bool     cmp;

  #define XCFG_SFX_DO_EXPAND(sfx) case XCFG_TID(sfx):
  switch (node->rtfi->tid) {
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
        memcpy(dst, &dst_str, node->rtfi->fld.size);
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

  if (cmp && !memcmp(dst, src, node->rtfi->fld.size))
    return XCFG_RET_SUCCESS;

  memcpy(dst, src, node->rtfi->fld.size);
  xcfg_node_raise_upd(node, data);

  return XCFG_RET_SUCCESS;
}

xcfg_ret
xcfg_node_get_value(xcfg_node *node, xcfg_ptr data, xcfg_ptr pval)
{
  xcfg_ptr src = xcfg_node_get_fld_ptr(node, data);
  xcfg_ptr dst;

  #define XCFG_SFX_DO_EXPAND(sfx) case XCFG_TID(sfx):
  switch (node->rtfi->tid) {
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
      memcpy(dst, &dst_str, node->rtfi->fld.size);

      /* set value to dst */
      memcpy(dst_str, src_str, src_sz);
      return XCFG_RET_SUCCESS;
    }
    default:
      return XCFG_RET_INVALID;
  }
  #undef XCFG_SFX_DO_EXPAND

  memcpy(dst, src, node->rtfi->fld.size);

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
xcfg_tree_create_node(xcfg_u32 upd_off, xcfg_rtfi *rtfi, xcfg_node *prev, ht_ctx *by_off, ht_ctx *by_key)
{
  xcfg_node *node = calloc(1, sizeof *node);

  node->rtfi = rtfi;
  node->data_fld_off = node->rtfi->fld.off;
  node->data_upd_off = upd_off;

  node->prev = prev;
  if (!node->prev) {
    /* branch start */
    asprintf(&node->data_fld_key, "%s", node->rtfi->key);
  } else {
    /* branch child */
    asprintf(&node->data_fld_key, "%s.%s", node->prev->data_fld_key, node->rtfi->key);
    node->data_fld_off += node->prev->data_fld_off;
    node->data_upd_off += node->prev->data_fld_off;
  }

  ht_set(by_off, ht_key_int(node->data_fld_off), node);
  ht_set(by_key, ht_key_str(node->data_fld_key), node);

  if (node->rtfi->tid == XCFG_TID_obj /* sub-nodes allowed only for `XCFG_TID_obj` fields */
   && node->rtfi->obj.rtti            /* there must be a type info for that object        */
   && node->rtfi->obj.rtti->nrtfi     /* there must be at least one field info            */
  ) {
    xcfg_rtti *obj_rtti = node->rtfi->obj.rtti;
    xcfg_u32   i;

    node->nnext = obj_rtti->nrtfi;
    node->next  = calloc(node->nnext, sizeof *node->next);
    for (i = 0; i < node->nnext; ++i) {
      node->next[i] =
        xcfg_tree_create_node(obj_rtti->upd.off, &obj_rtti->rtfi[i], node, by_off, by_key);
    }
  }

  return node;
}

xcfg_ret
xcfg_tree_build(xcfg_tree *tree, xcfg_rtti *rtti)
{
  xcfg_u32 i;

  if (!(rtti)
   || !(rtti->rtfi)
   || !(rtti->nrtfi)
  ) return XCFG_RET_INVALID;

  memset(tree, 0, sizeof *tree);

  tree->rtti = rtti;

  /* setup hashtables for fast node lookup */
  tree->by_off = ht_create(HT_KEY_INT, 32);
  tree->by_key = ht_create(HT_KEY_STR, 32);

  /* setup root field info */
  tree->rtfi.tid      = XCFG_TID_obj;
  tree->rtfi.obj.rtti = rtti;

  /* setup root and build a tree itself */
  tree->root.rtfi  = &tree->rtfi;
  tree->root.prev  = NULL;
  tree->root.nnext = rtti->nrtfi;
  tree->root.next  = calloc(tree->root.nnext, sizeof *tree->root.next);

  /* root is a proxy so nodes are created with `prev == NULL`
      but they are still related to root, so we link them manualy after creation */
  for (i = 0; i < tree->root.nnext; ++i) {
    tree->root.next[i] =
      xcfg_tree_create_node(rtti->upd.off, &rtti->rtfi[i], NULL, tree->by_off, tree->by_key);

    tree->root.next[i]->prev = &tree->root;
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

void
xcfg_tree_dispose(xcfg_tree *tree)
{
  xcfg_u32 i;

  if (!tree)
    return;

  for (i = 0; i < tree->root.nnext; ++i)
    xcfg_node_destroy(tree->root.next[i]);

  free(tree->root.next);

  ht_destroy(tree->by_off);
  ht_destroy(tree->by_key);

  memset(tree, 0, sizeof *tree);
}

static xcfg_ret
xcfg_node_tvs_do_dump(xcfg_node_tvs *curr, void *data)
{
  xcfg_node *node = curr->node;
  if (!node)
    return XCFG_RET_INVALID;

  logi(INDENT(".%s (%p)"), INDENTARG(curr->depth),
    node->rtfi->key, xcfg_node_get_fld_ptr(node, data));

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