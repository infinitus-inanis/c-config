#define _GNU_SOURCE /* strdup, qsort, etc.*/

#include "xcfg-file.h"
#include "utils/traverse.h"

#include <unistd.h>
#include <stdlib.h>
#include <memory.h>

#include <stdio.h>

#define logi(fmt, args...)         printf("[xcfg-file]: "fmt "\n", ## args)
#define logii(depth, fmt, args...) logi(INDENT(fmt), INDENTARG(depth), ## args)

static xcfg_u32 const __size_len = FIELD_SIZE_OF(xcfg_tree, size) * 2;

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

typedef struct {
  FILE     *stream;
  xcfg_ptr  data;
} save_ctx;

static xcfg_ret
xcfg_node_tvs_do_save(xcfg_node_tvs *curr, void *context)
{
  save_ctx  *ctx = context;
  xcfg_node *node = curr->node;

  if (!node)
    return XCFG_RET_FAILRUE;

  logii(curr->depth, ".%s = ''", node->key);

  return XCFG_RET_SUCCESS;
}

xcfg_ret
xcfg_file_save(xcfg_file *file, xcfg_tree *tree, xcfg_ptr data)
{
  save_ctx ctx;

  if (!file->path || !(*file->path))
    return XCFG_RET_FAILRUE;

  ctx.stream = fopen(file->path, "w");
  if (!ctx.stream)
    return XCFG_RET_FAILRUE;

  fseek(ctx.stream, 0, SEEK_SET);
  fprintf(ctx.stream, "0x%0*x\n", (xcfg_s32)(__size_len), tree->root.ref.size);
  fprintf(ctx.stream, "(%s) {\n", tree->root.key);

  xcfg_tree_tvs_depth_first(tree, xcfg_node_tvs_do_save, &ctx);

  fprintf(ctx.stream, "}\n");
  fclose(ctx.stream);

  return XCFG_RET_SUCCESS;
}

xcfg_ret
xcfg_file_load(xcfg_file *file, xcfg_tree *tree, xcfg_ptr data)
{
  (void)file;
  (void)tree;
  (void)data;
  return XCFG_RET_SUCCESS;
}