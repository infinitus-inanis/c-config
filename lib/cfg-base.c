#include "cfg-base.h"

#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <stddef.h>

cfg_ret
cfg_ent_set_generic(void          *cfg,
                    cfg_ent const *ent,
                    void    const *val,
                    cfg_upd const  upd_flag)
{
  void          *dst = (void    *)(cfg_u08 *)(cfg) + ent->fld.own.offset;
  cfg_upd       *upd = (cfg_upd *)(cfg_u08 *)(cfg) + ent->fld.upd.offset;
  void    const *src;
  bool           cmp;

  switch (ent->fld.own.type) {
    case CFG_FLD_TYPE_NAME(CFG_TID_s08): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_s16): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_s32): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_s64): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_u08): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_u16): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_u32): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_u64): {
      src = val;                         /* got pointer to basic type from op */
      cmp = true;                        /* basic types require comparison    */
    } break;

    case CFG_FLD_TYPE_NAME(CFG_TID_ptr): {
      src = *(cfg_ptr *)(val);           /* got pointer to pointer from op    */
      cmp = false;                       /* no reason to compare pointers     */
    } break;

    case CFG_FLD_TYPE_NAME(CFG_TID_obj): {
      src = *(cfg_obj *)(val);           /* got pointer to struct from op     */
      cmp = true;                        /* structures require comparison     */
    } break;

    default:
      return CFG_RET_INVALID;
  }

  if (cmp && !memcmp(dst, src, ent->fld.own.size))
    return CFG_RET_SUCCESS;

  memcpy(dst, src, ent->fld.own.size);
  *upd |= upd_flag;

  return CFG_RET_SUCCESS;
}

cfg_ret
cfg_ent_get_generic(void    const *cfg,
                    cfg_ent const *ent,
                    void          *val)
{
  void const *src = (cfg_u08 *)(cfg) + ent->fld.own.offset;
  void       *dst;

  switch (ent->fld.own.type) {
    case CFG_FLD_TYPE_NAME(CFG_TID_s08): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_s16): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_s32): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_s64): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_u08): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_u16): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_u32): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_u64): {
      dst = val;                         /* got pointer to basic type from op */
    } break;

    case CFG_FLD_TYPE_NAME(CFG_TID_ptr): {
      dst = *(cfg_ptr **)(val);          /* got pointer to pointer from op    */
    } break;

    case CFG_FLD_TYPE_NAME(CFG_TID_obj): {
      dst = *(cfg_obj **)(val);          /* got pointer to struct from op     */
    } break;

    default:
      return CFG_RET_INVALID;
  }

  memcpy(dst, src, ent->fld.own.size);

  return CFG_RET_SUCCESS;
}

static cfg_ret
cfg_ent_req_approve(cfg_ent const *ent, cfg_fld_type const req_type)
{
  cfg_fld_type const fld_type = ent->fld.own.type;
  cfg_u32      const fld_size = ent->fld.own.size;

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
    case CFG_FLD_TYPE_NAME(CFG_TID_u32): {
      if (fld_size != sizeof(cfg_s32))
        return CFG_RET_INVALID;
    } break;

    case CFG_FLD_TYPE_NAME(CFG_TID_s64): /* fallthrough */
    case CFG_FLD_TYPE_NAME(CFG_TID_u64): {
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


struct cfg_ctx {
  void           *data;
  cfg_ent const **ents;
  cfg_u32         ents_cnt;
};

cfg_ctx_t *
cfg_ctx_create(void           *data,
               cfg_ent const **ents,
               cfg_u32 const   ents_cnt)
{
  cfg_ctx_t *ctx;

  ctx = calloc(1, sizeof *ctx);
  if (!ctx)
    return NULL;

  ctx->data = data;
  ctx->ents = ents;
  ctx->ents_cnt = ents_cnt;

  return ctx;
}

void
cfg_ctx_destroy(cfg_ctx_t *ctx)
{
  if (!ctx)
    return;

  free(ctx);
}

static cfg_ent const *
cfg_ctx_select_ent(cfg_ctx_t const *ctx, char const *key)
{
  cfg_u32 ent_i;

  /* TODO (butsuk_d):
     optimize this using radix-tree */

  for (ent_i = 0; ent_i < ctx->ents_cnt; ++ent_i) {
    cfg_ent const *ent = ctx->ents[ent_i];

    if (!strcmp(key, ent->key))
      return ent;
  }

  return NULL;
}

static cfg_ent const *
cfg_ctx_select_ent_by_ref(cfg_ctx_t const *ctx, void const *fld)
{
  cfg_u32 fld_offset;
  cfg_u32 ent_i;

  /* TODO (butsuk_d):
     optimize this using hashtable */

  if (fld < ctx->data)
    return NULL;

  fld_offset = (cfg_u08 *)(fld) - (cfg_u08 *)(ctx->data);

  for (ent_i = 0; ent_i < ctx->ents_cnt; ++ent_i) {
    cfg_ent const *ent = ctx->ents[ent_i];

    if (ent->fld.own.offset == fld_offset)
      return ent;
  }

  return NULL;
}

#define CFG_ENT_SET(tid, ent) \
  ((DEFINE_CFG_ENT_OP_SET((*), tid))(ent)->ops.set)

#define CFG_ENT_GET(tid, ent) \
  ((DEFINE_CFG_ENT_OP_GET((*), tid))(ent)->ops.get)

#define DECLARE_CFG_CTX_OP_SET(tid)                         \
  DEFINE_CFG_CTX_OP_SET(tid)                                \
  {                                                         \
    cfg_ent const *ent;                                     \
    cfg_ret        ret;                                     \
                                                            \
    if (!ctx || !key)                                       \
      return CFG_RET_INVALID;                               \
                                                            \
    ent = cfg_ctx_select_ent(ctx, key);                     \
    if (!ent)                                               \
      return CFG_RET_UNKNOWN;                               \
                                                            \
    ret = cfg_ent_req_approve(ent, CFG_FLD_TYPE_NAME(tid)); \
    if (ret != CFG_RET_SUCCESS)                             \
      return ret;                                           \
                                                            \
    ret = CFG_ENT_SET(tid, ent)(ctx->data, ent, val);       \
    return ret;                                             \
  }

#define DECLARE_CFG_CTX_OP_GET(tid)                         \
  DEFINE_CFG_CTX_OP_GET(tid)                                \
  {                                                         \
    cfg_ent const *ent;                                     \
    cfg_ret        ret;                                     \
                                                            \
    if (!ctx || !key)                                       \
      return CFG_RET_INVALID;                               \
                                                            \
    ent = cfg_ctx_select_ent(ctx, key);                     \
    if (!ent)                                               \
      return CFG_RET_UNKNOWN;                               \
                                                            \
    ret = cfg_ent_req_approve(ent, CFG_FLD_TYPE_NAME(tid)); \
    if (ret != CFG_RET_SUCCESS)                             \
      return ret;                                           \
                                                            \
    ret = CFG_ENT_GET(tid, ent)(ctx->data, ent, val);       \
    return ret;                                             \
  }

EXPAND_CFG_TIDS(DECLARE_CFG_CTX_OP_SET, EOL_EMPTY)
EXPAND_CFG_TIDS(DECLARE_CFG_CTX_OP_GET, EOL_EMPTY)


#define DECLARE_CFG_CTX_OP_SET_BY_REF(tid)                  \
  DEFINE_CFG_CTX_OP_SET_BY_REF(tid)                         \
  {                                                         \
    cfg_ent const *ent;                                     \
    cfg_ret        ret;                                     \
                                                            \
    if (!ctx || !fld)                                       \
      return CFG_RET_INVALID;                               \
                                                            \
    ent = cfg_ctx_select_ent_by_ref(ctx, fld);              \
    if (!ent)                                               \
      return CFG_RET_UNKNOWN;                               \
                                                            \
    ret = cfg_ent_req_approve(ent, CFG_FLD_TYPE_NAME(tid)); \
    if (ret != CFG_RET_SUCCESS)                             \
      return ret;                                           \
                                                            \
    ret = CFG_ENT_SET(tid, ent)(ctx->data, ent, val);       \
    return ret;                                             \
  }

EXPAND_CFG_TIDS(DECLARE_CFG_CTX_OP_SET_BY_REF, EOL_EMPTY)