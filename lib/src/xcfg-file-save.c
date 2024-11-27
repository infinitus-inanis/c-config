#include "xcfg-file.h"
#include "xcfg-impl.h"
#include "xcfg-data.h"

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <errno.h>

#include <inttypes.h>

typedef void (* saver)(FILE *stream, xcfg_node *node, xcfg_ptr data, xcfg_u32 depth);

#define SAVER(sfx) CONCATENATE(XCFG_TYPE(sfx), _save)

#define XCFG_SFX_DO_EXPAND(sfx, fmt) \
  static void \
  SAVER(sfx)(FILE *stream, xcfg_node *node, xcfg_ptr data, xcfg_u32 depth) \
  { \
    xcfg_ptr pval = xcfg_node_get_fld_ptr(node, data); \
    XCFG_TYPE(sfx) val = *XCFG_TYPE_CAST(sfx, pval); \
    fprintf(stream, INDENT(".%s = " fmt ",\n"), INDENTARG(depth), node->rtfi->str, val); \
  }

  XCFG_SFX_EXPAND_s08("%"PRId8)
  XCFG_SFX_EXPAND_s16("%"PRId16)
  XCFG_SFX_EXPAND_s32("%"PRId32)
  XCFG_SFX_EXPAND_s64("%"PRId64)
  XCFG_SFX_EXPAND_u08("%"PRIu8)
  XCFG_SFX_EXPAND_u16("%"PRIu16)
  XCFG_SFX_EXPAND_u32("%"PRIu32)
  XCFG_SFX_EXPAND_u64("%"PRIu64)
  XCFG_SFX_EXPAND_f32("%f")
  XCFG_SFX_EXPAND_f64("%f")
  XCFG_SFX_EXPAND_str("\"%s\"")
#undef XCFG_SFX_DO_EXPAND

static saver lt_saver[XCFG_TID_COUNT] = {
#define XCFG_SFX_DO_EXPAND(sfx) \
  [XCFG_TID(sfx)] = SAVER(sfx),

  XCFG_SFX_EXPAND_val()
  XCFG_SFX_EXPAND_str()
#undef XCFG_SFX_DO_EXPAND
};

static void
save_node(FILE *stream, xcfg_node *node, xcfg_ptr data, xcfg_u32 depth)
{
  xcfg_u32 i;

  if (node->rtfi->tid != XCFG_TID_obj) {
    saver nsvr = lt_saver[node->rtfi->tid];
    if (!nsvr)
      return;

    nsvr(stream, node, data, depth);
    return;
  }

  fprintf(stream, INDENT(".%s = {\n"), INDENTARG(depth), node->rtfi->str);

  for (i = 0; i < node->nnext; ++i)
    save_node(stream, node->next[i], data, depth + 1);

  fprintf(stream, INDENT("},\n"), INDENTARG(depth));
}

xcfg_ret
xcfg_save_file(xcfg *cfg, const char *path)
{
  FILE      *stream;
  xcfg_tree *tree = &cfg->tree;

  if (!path || !path[0])
    return XCFG_RET_INVALID;

  stream = fopen(path, "w");
  if (!stream)
    return XCFG_RET_INVALID;

  fseek(stream, 0, SEEK_SET);
  { /* save header */
    fprintf(stream, "0x%x\n",   tree->rtti->size);
    fprintf(stream, "(%s) {\n", tree->rtti->name);
  }
  { /* save fields */
    xcfg_node *root = &tree->root;
    xcfg_u32   i;

    xcfg_lock_data(cfg);
    for (i = 0; i < root->nnext; ++i)
      save_node(stream, root->next[i], xcfg_get_data(cfg), 1);

    xcfg_unlock_data(cfg);
  }
  {
    fprintf(stream, "}\n");
  }
  fclose(stream);

  return XCFG_RET_SUCCESS;
}