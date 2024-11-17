#ifndef __CFG_TYPES_H__
#define __CFG_TYPES_H__

#include <stdint.h>
#include <stdbool.h>

#include <limits.h>

#define FIELD_OF(type, member)        ((type *)0)->member
#define FIELD_SIZE_OF(type, member)   (size_t)(sizeof(FIELD_OF(type, member)))
#define FIELD_OFFSET_OF(type, member) (size_t)(& FIELD_OF(type, member))

#define CONCAT_IMPL(s0, s1) s0##s1
#define CONCATENATE(s0, s1) CONCAT_IMPL(s0, s1)

#define EOL_EMPTY
#define EOL_COMMA     ,
#define EOL_SEMICOLON ;

typedef enum {
  CFG_RET_FAILRUE = -1,
  CFG_RET_SUCCESS =  0,

  CFG_RET_INVALID = CFG_RET_FAILRUE - 1,
  CFG_RET_UNKNOWN = CFG_RET_FAILRUE - 2,
} cfg_ret;

#define CFG_TID_s08 s08
#define CFG_TID_s16 s16
#define CFG_TID_s32 s32
#define CFG_TID_s64 s64
#define CFG_TID_u08 u08
#define CFG_TID_u16 u16
#define CFG_TID_u32 u32
#define CFG_TID_u64 u64
#define CFG_TID_f32 f32
#define CFG_TID_f64 f64
#define CFG_TID_ptr ptr
#define CFG_TID_obj obj

#define EXPAND_CFG_TIDS(gen, eol) \
  gen(CFG_TID_s08)eol             \
  gen(CFG_TID_s16)eol             \
  gen(CFG_TID_s32)eol             \
  gen(CFG_TID_s64)eol             \
  gen(CFG_TID_u08)eol             \
  gen(CFG_TID_u16)eol             \
  gen(CFG_TID_u32)eol             \
  gen(CFG_TID_u64)eol             \
  gen(CFG_TID_f32)eol             \
  gen(CFG_TID_f64)eol             \
  gen(CFG_TID_ptr)eol             \
  gen(CFG_TID_obj)eol


#define CFG_TYPE(id) CONCATENATE(cfg_, id)

typedef int8_t   CFG_TYPE(CFG_TID_s08);
typedef int16_t  CFG_TYPE(CFG_TID_s16);
typedef int32_t  CFG_TYPE(CFG_TID_s32);
typedef int64_t  CFG_TYPE(CFG_TID_s64);

typedef uint8_t  CFG_TYPE(CFG_TID_u08);
typedef uint16_t CFG_TYPE(CFG_TID_u16);
typedef uint32_t CFG_TYPE(CFG_TID_u32);
typedef uint64_t CFG_TYPE(CFG_TID_u64);

typedef float    CFG_TYPE(CFG_TID_f32);
typedef double   CFG_TYPE(CFG_TID_f64);

typedef void *   CFG_TYPE(CFG_TID_ptr);
typedef void *   CFG_TYPE(CFG_TID_obj);

typedef cfg_u64 cfg_upd;
#define CFG_UPD(id) ((cfg_upd)(1) << (cfg_upd)(id))

#define CFG_FLD_TYPE_NAME(tid) CONCATENATE(CFG_FLD_TYPE_, tid)

typedef enum {
  EXPAND_CFG_TIDS(CFG_FLD_TYPE_NAME, EOL_COMMA)
} cfg_fld_type;

typedef struct {
  cfg_fld_type type : 8;
  cfg_u32      size;
  cfg_u32      offset;
} cfg_fld_info;

typedef struct {
  cfg_u08 id;
  cfg_u32 size;
  cfg_u32 offset;
} cfg_upd_info;

typedef struct cfg_info cfg_info;
struct cfg_info {
  char const *key;

  cfg_fld_info fld;
  cfg_upd_info upd;

  struct {
    cfg_info *data;
    cfg_u32   size;
  } children;
};

#define CFG_INFO_0(cfg_type, fld_path, fld_tid, upd_path, upd_id, cfg_children, cfg_nchildren) \
  {                                                                                            \
    .key = #fld_path,                                                                          \
    .fld = {                                                                                   \
      .type   = CFG_FLD_TYPE_NAME(fld_tid),                                                    \
      .size   = FIELD_SIZE_OF(cfg_type, fld_path),                                             \
      .offset = FIELD_OFFSET_OF(cfg_type, fld_path),                                           \
    },                                                                                         \
    .upd = {                                                                                   \
      .id     = upd_id,                                                                        \
      .size   = FIELD_SIZE_OF(cfg_type, upd_path),                                             \
      .offset = FIELD_OFFSET_OF(cfg_type, upd_path),                                           \
    },                                                                                         \
    .children = {                                                                              \
      .data = cfg_children,                                                                    \
      .size = cfg_nchildren,                                                                   \
    }                                                                                          \
  }

#define CFG_INFO_1(cfg_type, fld_path, fld_tid, upd_path, upd_id) \
  CFG_INFO_0(cfg_type, fld_path, fld_tid, upd_path, upd_id, NULL, 0)

typedef struct cfg_ctx cfg_ctx;

cfg_ctx *
cfg_ctx_create(void     *data,
               cfg_u32   size,
               cfg_info *infos,
               cfg_u32   infos_n);

void
cfg_ctx_destroy(cfg_ctx *ctx);

cfg_ret
cfg_ctx_bind_file(cfg_ctx *ctx, char const *path);

cfg_ret
cfg_ctx_save_file(cfg_ctx *ctx);

cfg_ret
cfg_ctx_load_file(cfg_ctx *ctx);

#define CFG_CTX_OP_SET_NAME(tid) CONCATENATE(cfg_ctx_set_by_key_, tid)
#define CFG_CTX_OP_GET_NAME(tid) CONCATENATE(cfg_ctx_get_by_key_, tid)

#define DEFINE_CFG_CTX_OP_SET(tid)                   \
  cfg_ret                                            \
  CFG_CTX_OP_SET_NAME(tid)(cfg_ctx             *ctx, \
                           char          const *key, \
                           CFG_TYPE(tid) const  val)

#define DEFINE_CFG_CTX_OP_GET(tid)                   \
  cfg_ret                                            \
  CFG_CTX_OP_GET_NAME(tid)(cfg_ctx       const *ctx, \
                           char          const *key, \
                           CFG_TYPE(tid)       *val)

EXPAND_CFG_TIDS(DEFINE_CFG_CTX_OP_SET, EOL_SEMICOLON)
EXPAND_CFG_TIDS(DEFINE_CFG_CTX_OP_GET, EOL_SEMICOLON)


#define CFG_CTX_OP_SET_BY_REF_NAME(tid) CONCATENATE(cfg_ctx_set_by_ref_, tid)

#define DEFINE_CFG_CTX_OP_SET_BY_REF(tid)                   \
  cfg_ret                                                   \
  CFG_CTX_OP_SET_BY_REF_NAME(tid)(cfg_ctx             *ctx, \
                                  void          const *fld, \
                                  CFG_TYPE(tid) const  val)

EXPAND_CFG_TIDS(DEFINE_CFG_CTX_OP_SET_BY_REF, EOL_SEMICOLON)


#endif//__CFG_TYPES_H__
