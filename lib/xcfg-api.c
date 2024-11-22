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
  struct {
    xcfg_str name;
    xcfg_ptr data;
    xcfg_u32 size;
  } type;

  xcfg_tree tree;
  xcfg_file file;
};

xcfg *
xcfg_create(xcfg_str  type_name,
            xcfg_u32  type_size,
            xcfg_fld *root_pfld,
            xcfg_u32  root_nfld)
{
  xcfg     *ctx;
  xcfg_ret  ret;

  if (!type_name || !type_size
   || !root_pfld || !root_nfld
  ) return NULL;

  ctx = calloc(1, sizeof *ctx);
  if (!ctx)
    return NULL;

  ctx->type.name = type_name;
  ctx->type.size = type_size;

  ret = xcfg_tree_build(&ctx->tree, type_name, type_size, root_pfld, root_nfld);
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
  logi("type.name: %s", ctx->type.name);
  logi("type.data: %p", ctx->type.data);
  logi("type.size: %u", ctx->type.size);
  xcfg_tree_dump(&ctx->tree, ctx->type.data);
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
  return xcfg_file_set_path(&ctx->file, path);
}

xcfg_ret
xcfg_save_file(xcfg *ctx)
{
  return xcfg_file_save(&ctx->file, &ctx->tree, ctx->type.data);
}

xcfg_ret
xcfg_load_file(xcfg *ctx)
{
  return xcfg_file_load(&ctx->file, &ctx->tree, ctx->type.data);
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

  return xcfg_tree_get_node_by_off(&ctx->tree, off);
}

static xcfg_node *
xcfg_get_node_by_key(xcfg *ctx, xcfg_str key)
{
  return xcfg_tree_get_node_by_key(&ctx->tree, key);
}

static xcfg_ret
xcfg_set_by_ref_impl(xcfg *ctx, xcfg_ptr ref, xcfg_fld_type type, xcfg_ptr pval)
{
  xcfg_node *node;
  xcfg_ret   ret;

  if (!ctx || !ref)
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

  if (!ctx || !key)
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

  if (!ctx || !key)
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

  XCFG_SFX_EXPAND_all()
#undef XCFG_SFX_DO_EXPAND


#define XCFG_SFX_DO_EXPAND(sfx) \
  xcfg_ret \
  XCFG_SET_BY_KEY(sfx)(xcfg *ctx, xcfg_str key, XCFG_TYPE(sfx) val) \
  { return xcfg_set_by_key_impl(ctx, key, XCFG_FLD_TYPE(sfx), (xcfg_ptr)(&val)); }

  XCFG_SFX_EXPAND_all()
#undef XCFG_SFX_DO_EXPAND


#define XCFG_SFX_DO_EXPAND(sfx) \
  xcfg_ret \
  XCFG_GET_BY_KEY(sfx)(xcfg *ctx, xcfg_str key, XCFG_TYPE(sfx) *pval) \
  { return xcfg_get_by_key_impl(ctx, key, XCFG_FLD_TYPE(sfx), (xcfg_ptr)(pval)); }

  XCFG_SFX_EXPAND_val()
  XCFG_SFX_EXPAND_ptr()
  XCFG_SFX_EXPAND_str()
#undef XCFG_SFX_DO_EXPAND


xcfg_ret
XCFG_GET_BY_KEY(XCFG_SFX_obj)(xcfg *ctx, xcfg_str key, xcfg_obj obj)
{ return xcfg_get_by_key_impl(ctx, key, XCFG_FLD_TYPE(XCFG_SFX_obj), (xcfg_ptr)(obj)); }