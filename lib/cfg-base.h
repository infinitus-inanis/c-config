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


#define CFG_UPD_FLAG(id) (UINT64_C(1) << UINT64_C(id))
#define CFG_UPD_FLAG_ALL (UINT64_MAX)

typedef cfg_u64 cfg_upd;


#define CFG_FLD_TYPE_NAME(tid) CONCATENATE(CFG_FLD_TYPE_, tid)

typedef enum {
  EXPAND_CFG_TIDS(CFG_FLD_TYPE_NAME, EOL_COMMA)
} cfg_fld_type;

typedef struct {
  cfg_fld_type type;
  cfg_u32      size;
  cfg_u32      offset;
} cfg_fld;

#define CFG_FLD(owner_type, fld_path, fld_tid)       \
  (cfg_fld) {                                        \
    .type   = CFG_FLD_TYPE_NAME(fld_tid),            \
    .size   = FIELD_SIZE_OF(owner_type, fld_path),   \
    .offset = FIELD_OFFSET_OF(owner_type, fld_path), \
  }


typedef struct {
  const char *key; /* unique entry key        */

  struct {
    cfg_fld own;   /* entry own field info    */
    cfg_fld upd;   /* entry update field info */
  } fld;

  struct {
    void *set;     /* entry setter            */
    void *get;     /* entry getter            */
  } ops;
} cfg_ent;

cfg_ret
cfg_ent_set_generic(void          *cfg,
                    cfg_ent const *ent,
                    void    const *val,
                    cfg_upd const  upd_flag);

cfg_ret
cfg_ent_get_generic(void    const *cfg,
                    cfg_ent const *ent,
                    void          *val);


#define CFG_ENT_NAME(sfx)        CONCATENATE(__cfg_ent_,     sfx)
#define CFG_ENT_OP_SET_NAME(sfx) CONCATENATE(__cfg_ent_set_, sfx)
#define CFG_ENT_OP_GET_NAME(sfx) CONCATENATE(__cfg_ent_get_, sfx)

#define DEFINE_CFG_ENT_OP_SET(name, tid) \
  cfg_ret                                \
  name(void                *cfg,         \
       cfg_ent       const *ent,         \
       CFG_TYPE(tid) const  val)

#define DEFINE_CFG_ENT_OP_GET(name, tid) \
  cfg_ret                                \
  name(void          const *cfg,         \
       cfg_ent       const *ent,         \
       CFG_TYPE(tid)       *val)

#define DECLARE_CFG_ENT(cfg_type, fld_path, fld_tid, upd_path, upd_flag) \
  static inline                                                          \
  DEFINE_CFG_ENT_OP_SET(CFG_ENT_OP_SET_NAME(upd_flag), fld_tid)          \
  {                                                                      \
    return cfg_ent_set_generic(cfg, ent, (void *)(&val), upd_flag);      \
  }                                                                      \
                                                                         \
  static inline                                                          \
  DEFINE_CFG_ENT_OP_GET(CFG_ENT_OP_GET_NAME(upd_flag), fld_tid)          \
  {                                                                      \
    return cfg_ent_get_generic(cfg, ent, (void *)(val));                 \
  }                                                                      \
                                                                         \
  static cfg_ent CFG_ENT_NAME(upd_flag) =                                \
  {                                                                      \
    .key = #fld_path,                                                    \
    .fld = {                                                             \
      .own = CFG_FLD(cfg_type, fld_path, fld_tid),                       \
      .upd = CFG_FLD(cfg_type, upd_path, CFG_TID_u64),                   \
    },                                                                   \
    .ops = {                                                             \
      .set = (cfg_ptr)(CFG_ENT_OP_SET_NAME(upd_flag)),                   \
      .get = (cfg_ptr)(CFG_ENT_OP_GET_NAME(upd_flag)),                   \
    },                                                                   \
  };

#define DECLARE_CFG_ENT_PTR(cfg_type, fld_path, fld_type, upd_path, upd_flag) \
  &CFG_ENT_NAME(upd_flag),

#define DECLARE_CFG_ENTS(name, table) \
  table(DECLARE_CFG_ENT)              \
  static cfg_ent const *name[] = {    \
    table(DECLARE_CFG_ENT_PTR)        \
  };


typedef struct cfg_ctx cfg_ctx_t;

cfg_ctx_t *
cfg_ctx_create(void           *data,
               cfg_ent const **ents,
               cfg_u32 const   ents_cnt);

void
cfg_ctx_destroy(cfg_ctx_t *ctx);

cfg_ret
cfg_ctx_file_bind(cfg_ctx_t *ctx, char const *path);

cfg_ret
cfg_ctx_file_save(cfg_ctx_t *ctx);

cfg_ret
cfg_ctx_file_load(cfg_ctx_t *ctx);


#define CFG_CTX_OP_SET_NAME(tid) CONCATENATE(cfg_ctx_set_by_key_, tid)
#define CFG_CTX_OP_GET_NAME(tid) CONCATENATE(cfg_ctx_get_by_key_, tid)

#define DEFINE_CFG_CTX_OP_SET(tid)                   \
  cfg_ret                                            \
  CFG_CTX_OP_SET_NAME(tid)(cfg_ctx_t           *ctx, \
                           char          const *key, \
                           CFG_TYPE(tid) const  val)

#define DEFINE_CFG_CTX_OP_GET(tid)                   \
  cfg_ret                                            \
  CFG_CTX_OP_GET_NAME(tid)(cfg_ctx_t     const *ctx, \
                           char          const *key, \
                           CFG_TYPE(tid)       *val)

EXPAND_CFG_TIDS(DEFINE_CFG_CTX_OP_SET, EOL_SEMICOLON)
EXPAND_CFG_TIDS(DEFINE_CFG_CTX_OP_GET, EOL_SEMICOLON)


#define CFG_CTX_OP_SET_BY_REF_NAME(tid) CONCATENATE(cfg_ctx_set_by_ref_, tid)

#define DEFINE_CFG_CTX_OP_SET_BY_REF(tid)                   \
  cfg_ret                                                   \
  CFG_CTX_OP_SET_BY_REF_NAME(tid)(cfg_ctx_t           *ctx, \
                                  void          const *fld, \
                                  CFG_TYPE(tid) const  val)

EXPAND_CFG_TIDS(DEFINE_CFG_CTX_OP_SET_BY_REF, EOL_SEMICOLON)


#endif//__CFG_TYPES_H__
