#define _GNU_SOURCE /* strdup, qsort, etc.*/

#include "xcfg-api.h"
#include "xcfg-tree.h"
#include "xcfg-file.h"

#include <unistd.h>
#include <stdlib.h>
#include <memory.h>

#include <stdio.h>
#define logi(fmt, args...) printf("[xcfg]: "fmt "\n", ## args)

struct xcfg {
  xcfg_ptr   data;
  xcfg_rtti *rtti;
  xcfg_tree  tree;
  xcfg_file  file;
};

xcfg *
xcfg_create(xcfg_rtti *rtti)
{
  xcfg     *ctx;
  xcfg_ret  ret;

  if (!rtti)
    return NULL;

  ctx = calloc(1, sizeof *ctx);
  if (!ctx)
    return NULL;

  ctx->rtti = rtti;

  ret = xcfg_tree_build(&ctx->tree, ctx->rtti);
  if (ret < 0)
    goto error;

  return ctx;

error:
  xcfg_destroy(ctx);
  return NULL;
}

void
xcfg_destroy(xcfg *ctx)
{
  if (!ctx)
    return;

  xcfg_file_dispose(&ctx->file);
  xcfg_tree_dispose(&ctx->tree);
  free(ctx);
}

void
xcfg_dump(xcfg *ctx)
{
  logi("data: %p",      ctx->data);
  logi("type.name: %s", ctx->rtti->name);
  logi("type.size: %u", ctx->rtti->size);
  xcfg_tree_dump(&ctx->tree, ctx->data);
}

xcfg_ret
xcfg_set_data(xcfg *ctx, xcfg_ptr data, xcfg_u32 size)
{
  if (ctx->rtti->size != size)
    return XCFG_RET_INVALID;

  ctx->data = data;
  return XCFG_RET_SUCCESS;
}

xcfg_ptr
xcfg_get_data(xcfg *ctx)
{
  return ctx->data;
}

xcfg_ret
xcfg_bind_file(xcfg *ctx, xcfg_str path)
{
  return xcfg_file_set_path(&ctx->file, path);
}

xcfg_ret
xcfg_save_file(xcfg *ctx)
{
  return xcfg_file_save(&ctx->file, &ctx->tree, ctx->data);
}

xcfg_ret
xcfg_load_file(xcfg *ctx)
{
  return xcfg_file_load(&ctx->file, &ctx->tree, ctx->data);
}

xcfg_ret
xcfg_monitor_file(xcfg *ctx)
{
  return xcfg_file_monitor(&ctx->file, &ctx->tree, ctx->data);
}

static xcfg_node *
xcfg_get_node_by_ref(xcfg *ctx, xcfg_ptr ref)
{
  xcfg_u32 off;

  /* reference can't be higher in address space */
  if (ref < ctx->data)
    return NULL;

  off = (xcfg_u08 *)(ref) - (xcfg_u08 *)(ctx->data);

  /* validate offset bounds */
  if (off >= ctx->rtti->size)
    return NULL;

  return xcfg_tree_get_node_by_off(&ctx->tree, off);
}

static xcfg_node *
xcfg_get_node_by_key(xcfg *ctx, xcfg_str key)
{
  return xcfg_tree_get_node_by_key(&ctx->tree, key);
}

static xcfg_ret
xcfg_set_by_ref_impl(xcfg *ctx, xcfg_ptr ref, xcfg_tid tid, xcfg_ptr pval)
{
  xcfg_node *node;
  xcfg_ret   ret;

  if (!ctx || !ref)
    return XCFG_RET_INVALID;

  node = xcfg_get_node_by_ref(ctx, ref);
  if (!node)
    return XCFG_RET_UNKNOWN;

  ret = xcfg_node_type_check(node, tid);
  if (ret < 0)
    return ret;

  return xcfg_node_set_value(node, ctx->data, pval);
}

static xcfg_ret
xcfg_set_by_key_impl(xcfg *ctx, xcfg_str key, xcfg_tid tid, xcfg_ptr pval)
{
  xcfg_node *node;
  xcfg_ret   ret;

  if (!ctx || !key)
    return XCFG_RET_INVALID;

  node = xcfg_get_node_by_key(ctx, key);
  if (!node)
    return XCFG_RET_UNKNOWN;

  ret = xcfg_node_type_check(node, tid);
  if (ret < 0)
    return ret;

  return xcfg_node_set_value(node, ctx->data, pval);
}

static xcfg_ret
xcfg_get_by_key_impl(xcfg *ctx, xcfg_str key, xcfg_tid tid, xcfg_ptr pval)
{
  xcfg_node *node;
  xcfg_ret   ret;

  if (!ctx || !key)
    return XCFG_RET_INVALID;

  node = xcfg_get_node_by_key(ctx, key);
  if (!node)
    return XCFG_RET_UNKNOWN;

  ret = xcfg_node_type_check(node, tid);
  if (ret < 0)
    return ret;

  return xcfg_node_get_value(node, ctx->data, pval);
}

#define XCFG_SFX_DO_EXPAND(sfx) \
  xcfg_ret \
  XCFG_SET_BY_REF(sfx)(xcfg *ctx, xcfg_ptr ref, XCFG_TYPE(sfx) val) \
  { return xcfg_set_by_ref_impl(ctx, ref, XCFG_TID(sfx), (xcfg_ptr)(&val)); }

  XCFG_SFX_EXPAND_all()
#undef XCFG_SFX_DO_EXPAND


#define XCFG_SFX_DO_EXPAND(sfx) \
  xcfg_ret \
  XCFG_SET_BY_KEY(sfx)(xcfg *ctx, xcfg_str key, XCFG_TYPE(sfx) val) \
  { return xcfg_set_by_key_impl(ctx, key, XCFG_TID(sfx), (xcfg_ptr)(&val)); }

  XCFG_SFX_EXPAND_all()
#undef XCFG_SFX_DO_EXPAND


#define XCFG_SFX_DO_EXPAND(sfx) \
  xcfg_ret \
  XCFG_GET_BY_KEY(sfx)(xcfg *ctx, xcfg_str key, XCFG_TYPE(sfx) *pval) \
  { return xcfg_get_by_key_impl(ctx, key, XCFG_TID(sfx), (xcfg_ptr)(pval)); }

  XCFG_SFX_EXPAND_val()
  XCFG_SFX_EXPAND_ptr()
  XCFG_SFX_EXPAND_str()
#undef XCFG_SFX_DO_EXPAND


xcfg_ret
XCFG_GET_BY_KEY(XCFG_SFX_obj)(xcfg *ctx, xcfg_str key, xcfg_obj obj)
{ return xcfg_get_by_key_impl(ctx, key, XCFG_TID(XCFG_SFX_obj), (xcfg_ptr)(obj)); }