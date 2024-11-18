#ifndef __CFG_TYPES_H__
#define __CFG_TYPES_H__

#include <stdint.h>
#include <stdbool.h>

#include <limits.h>
#include <bits/wordsize.h>

#define FIELD_OF(type, member)        ((type *)0)->member
#define FIELD_SIZE_OF(type, member)   (size_t)(sizeof(FIELD_OF(type, member)))
#define FIELD_OFFSET_OF(type, member) (size_t)(& FIELD_OF(type, member))

#define CONCAT_IMPL(s0, s1) s0##s1
#define CONCATENATE(s0, s1) CONCAT_IMPL(s0, s1)

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

/* WARNING: (runtime-only feature)
   this type is used to get/set address of a pointer */
#define CFG_TID_ptr ptr

/* WARNING: (requires manual memory management)
   1. On 'get' a new string will be allocated for user
      and config value will be copied into it.
   2. On 'set' a config value will be realloced (if needed)
      and passed user  value will be copied into it.
      Be aware that holding on to original string pointer
      is dangerous due to possible reallocation.  */
#define CFG_TID_str str

/* WARNING: (runtime-only feature)
   this type is used to get/set value of a structure as is
   and for declaring nested config structures */
#define CFG_TID_obj obj

/* TODO (butsuk_d): CFG_TID_hex ? */

#define EXPAND_CFG_TIDS_BASE(gen) \
  gen(CFG_TID_s08)                \
  gen(CFG_TID_s16)                \
  gen(CFG_TID_s32)                \
  gen(CFG_TID_s64)                \
  gen(CFG_TID_u08)                \
  gen(CFG_TID_u16)                \
  gen(CFG_TID_u32)                \
  gen(CFG_TID_u64)                \
  gen(CFG_TID_f32)                \
  gen(CFG_TID_f64)

#define EXPAND_CFG_TIDS_PTRS(gen) \
  gen(CFG_TID_ptr)                \
  gen(CFG_TID_str)                \

#define EXPAND_CFG_TIDS_ALL(gen)  \
  EXPAND_CFG_TIDS_BASE(gen)       \
  EXPAND_CFG_TIDS_PTRS(gen)       \
  gen(CFG_TID_obj)


#define CFG_TYPE(id) CONCATENATE(cfg_, id)

typedef int8_t    CFG_TYPE(CFG_TID_s08);
typedef int16_t   CFG_TYPE(CFG_TID_s16);
typedef int32_t   CFG_TYPE(CFG_TID_s32);
typedef int64_t   CFG_TYPE(CFG_TID_s64);

typedef uint8_t   CFG_TYPE(CFG_TID_u08);
typedef uint16_t  CFG_TYPE(CFG_TID_u16);
typedef uint32_t  CFG_TYPE(CFG_TID_u32);
typedef uint64_t  CFG_TYPE(CFG_TID_u64);

typedef float     CFG_TYPE(CFG_TID_f32);
typedef double    CFG_TYPE(CFG_TID_f64);

typedef void *    CFG_TYPE(CFG_TID_ptr);
typedef char *    CFG_TYPE(CFG_TID_str);

typedef void *    CFG_TYPE(CFG_TID_obj);

typedef cfg_u64 cfg_upd;
#define CFG_UPD(id) ((cfg_upd)(1) << (cfg_upd)(id))

#define CFG_FLD_TYPE_NAME(tid)   CONCATENATE(CFG_FLD_TYPE_, tid)
#define DEFINE_CFG_FLD_TYPE(tid) CFG_FLD_TYPE_NAME(tid),

typedef enum {
  EXPAND_CFG_TIDS_ALL(DEFINE_CFG_FLD_TYPE)
} cfg_fld_type;

typedef struct {
  cfg_u32 type :  8; /* cfg_fld_type */
  cfg_u32 off  : 24;
  cfg_u32 size;
} cfg_fld_info;

typedef struct {
  cfg_u32 id  :  8;
  cfg_u32 off : 24;
  cfg_u32 size;
} cfg_upd_info;

typedef struct cfg_info cfg_info;
struct cfg_info {
  char const *key;

  cfg_fld_info fld;
  cfg_upd_info upd;

  cfg_info *subs;
  cfg_u32   subs_n;
};

#define CFG_INFO_0(cfg_type, fld_path, fld_tid, upd_path, upd_id, cfg_subs, cfg_subs_n) \
  {                                                                                     \
    .key = #fld_path,                                                                   \
    .fld = {                                                                            \
      .type = CFG_FLD_TYPE_NAME(fld_tid),                                               \
      .off  = FIELD_OFFSET_OF(cfg_type, fld_path),                                      \
      .size = FIELD_SIZE_OF(cfg_type, fld_path),                                        \
    },                                                                                  \
    .upd = {                                                                            \
      .id   = upd_id,                                                                   \
      .off  = FIELD_OFFSET_OF(cfg_type, upd_path),                                      \
      .size = FIELD_SIZE_OF(cfg_type, upd_path),                                        \
    },                                                                                  \
    .subs = cfg_subs,                                                                   \
    .subs_n = cfg_subs_n,                                                               \
  }

#define CFG_INFO_1(cfg_type, fld_path, fld_tid, upd_path, upd_id) \
  CFG_INFO_0(cfg_type, fld_path, fld_tid, upd_path, upd_id, NULL, 0)

typedef struct cfg_ctx cfg_ctx;

cfg_ctx *
cfg_ctx_create(void     *data,
               cfg_u32   size,
               cfg_info *infos,
               cfg_u32   infos_n,
               cfg_u32   infos_total_n);

void
cfg_ctx_destroy(cfg_ctx *ctx);

cfg_ret
cfg_ctx_bind_file(cfg_ctx *ctx, char const *path);

cfg_ret
cfg_ctx_save_file(cfg_ctx *ctx);

cfg_ret
cfg_ctx_load_file(cfg_ctx *ctx);


#define CFG_CTX_SET_BY_REF_NAME(tid) CONCATENATE(cfg_ctx_set_by_ref_, tid)
#define CFG_CTX_SET_BY_KEY_NAME(tid) CONCATENATE(cfg_ctx_set_by_key_, tid)
#define CFG_CTX_GET_BY_KEY_NAME(tid) CONCATENATE(cfg_ctx_get_by_key_, tid)

#define DEFINE_CFG_CTX_SET_BY_REF(tid)                   \
  cfg_ret                                                \
  CFG_CTX_SET_BY_REF_NAME(tid)(cfg_ctx             *ctx, \
                               void          const *ref, \
                               CFG_TYPE(tid) const  val);

#define DEFINE_CFG_CTX_SET_BY_KEY(tid)                   \
  cfg_ret                                                \
  CFG_CTX_SET_BY_KEY_NAME(tid)(cfg_ctx             *ctx, \
                               char          const *key, \
                               CFG_TYPE(tid) const  val);

#define DEFINE_CFG_CTX_GET_BY_KEY(tid)                   \
  cfg_ret                                                \
  CFG_CTX_GET_BY_KEY_NAME(tid)(cfg_ctx       const *ctx, \
                               char          const *key, \
                               CFG_TYPE(tid)       *val);

EXPAND_CFG_TIDS_ALL(DEFINE_CFG_CTX_SET_BY_REF)
EXPAND_CFG_TIDS_ALL(DEFINE_CFG_CTX_SET_BY_KEY)

EXPAND_CFG_TIDS_BASE(DEFINE_CFG_CTX_GET_BY_KEY)
EXPAND_CFG_TIDS_PTRS(DEFINE_CFG_CTX_GET_BY_KEY)

/* WARNING: getter for `CFG_TID_obj` is a special case
            as we do not want a pointer to pointer to object */
cfg_ret
CFG_CTX_GET_BY_KEY_NAME(CFG_TID_obj)(cfg_ctx       *ctx,
                                     char    const *key,
                                     cfg_obj        obj);

#endif//__CFG_TYPES_H__
