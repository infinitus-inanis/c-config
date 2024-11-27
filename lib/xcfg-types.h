#ifndef __XCFG_TYPES_H__
#define __XCFG_TYPES_H__

#include "xcfg-utils.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

XCFG_EXPORT_ENTER

typedef enum {
  XCFG_RET_FAILRUE = -1,
  XCFG_RET_SUCCESS =  0,

  XCFG_RET_INVALID = XCFG_RET_FAILRUE - 1,
  XCFG_RET_UNKNOWN = XCFG_RET_FAILRUE - 2,
} xcfg_ret;


/******************************** *
 * Suffixes for underlying types. *
 * Basicaly are compile-time type ids. */

#define XCFG_SFX_s08 s08
#define XCFG_SFX_s16 s16
#define XCFG_SFX_s32 s32
#define XCFG_SFX_s64 s64
#define XCFG_SFX_u08 u08
#define XCFG_SFX_u16 u16
#define XCFG_SFX_u32 u32
#define XCFG_SFX_u64 u64
#define XCFG_SFX_f32 f32
#define XCFG_SFX_f64 f64

/*  WARNING: (runtime-only feature)
    This type is used to get/set address of a pointer. */
#define XCFG_SFX_ptr ptr

/*  WARNING: (requires manual memory management)
    1. On 'get' a new string will be allocated for user
      and config value will be copied into it.
    2. On 'set' a config value will be realloced (if needed)
      and passed user value will be copied into it.
      Be aware that holding on to original string pointer
      is dangerous due to possible reallocation. */
#define XCFG_SFX_str str

/*  WARNING: (runtime-only feature)
    This type is used to get/set value of a structure as is
    and for managing nested config structures */
#define XCFG_SFX_obj obj


/********************************************
 * Expand-macros for `XCFG_SFX_*` values */

#define XCFG_SFX_EXPAND_s08(args...) XCFG_SFX_DO_EXPAND(XCFG_SFX_s08, ## args)
#define XCFG_SFX_EXPAND_s16(args...) XCFG_SFX_DO_EXPAND(XCFG_SFX_s16, ## args)
#define XCFG_SFX_EXPAND_s32(args...) XCFG_SFX_DO_EXPAND(XCFG_SFX_s32, ## args)
#define XCFG_SFX_EXPAND_s64(args...) XCFG_SFX_DO_EXPAND(XCFG_SFX_s64, ## args)
#define XCFG_SFX_EXPAND_u08(args...) XCFG_SFX_DO_EXPAND(XCFG_SFX_u08, ## args)
#define XCFG_SFX_EXPAND_u16(args...) XCFG_SFX_DO_EXPAND(XCFG_SFX_u16, ## args)
#define XCFG_SFX_EXPAND_u32(args...) XCFG_SFX_DO_EXPAND(XCFG_SFX_u32, ## args)
#define XCFG_SFX_EXPAND_u64(args...) XCFG_SFX_DO_EXPAND(XCFG_SFX_u64, ## args)
#define XCFG_SFX_EXPAND_f32(args...) XCFG_SFX_DO_EXPAND(XCFG_SFX_f32, ## args)
#define XCFG_SFX_EXPAND_f64(args...) XCFG_SFX_DO_EXPAND(XCFG_SFX_f64, ## args)
#define XCFG_SFX_EXPAND_ptr(args...) XCFG_SFX_DO_EXPAND(XCFG_SFX_ptr, ## args)
#define XCFG_SFX_EXPAND_str(args...) XCFG_SFX_DO_EXPAND(XCFG_SFX_str, ## args)
#define XCFG_SFX_EXPAND_obj(args...) XCFG_SFX_DO_EXPAND(XCFG_SFX_obj, ## args)

/* Expands to signed types. */
#define XCFG_SFX_EXPAND_sxx(args...) \
  XCFG_SFX_EXPAND_s08(args) \
  XCFG_SFX_EXPAND_s16(args) \
  XCFG_SFX_EXPAND_s32(args) \
  XCFG_SFX_EXPAND_s64(args)

/* Expands to unsigned types. */
#define XCFG_SFX_EXPAND_uxx(args...) \
  XCFG_SFX_EXPAND_u08(args) \
  XCFG_SFX_EXPAND_u16(args) \
  XCFG_SFX_EXPAND_u32(args) \
  XCFG_SFX_EXPAND_u64(args)

/* Expands to float types. */
#define XCFG_SFX_EXPAND_fxx(args...) \
  XCFG_SFX_EXPAND_f32(args) \
  XCFG_SFX_EXPAND_f64(args)

/* Expands to signed/unisgned/float types. */
#define XCFG_SFX_EXPAND_val(args...) \
  XCFG_SFX_EXPAND_sxx(args) \
  XCFG_SFX_EXPAND_uxx(args) \
  XCFG_SFX_EXPAND_fxx(args)

/* Expands to all available types */
#define XCFG_SFX_EXPAND_all(args...) \
  XCFG_SFX_EXPAND_val(args) \
  XCFG_SFX_EXPAND_ptr(args) \
  XCFG_SFX_EXPAND_str(args) \
  XCFG_SFX_EXPAND_obj(args)


/******************************************
 * Type definitions for underlying types. */

/* Resolves type suffix to type definition */
#define XCFG_TYPE(sfx)           CONCATENATE(xcfg_, sfx)

/* Resolves to size of a type by it's suffix */
#define XCFG_TYPE_SIZE(sfx)      (sizeof(XCFG_TYPE(sfx)))

/* Pointer cast to xcfg pointer type by it's suffix */
#define XCFG_TYPE_CAST(sfx, ptr) (XCFG_TYPE(sfx) *)(ptr)

/* Creates temporary array of bytes of size
    equal to size of xcfg type by it's suffix */
#define XCFG_TYPE_TEMP(sfx)      (xcfg_u08[XCFG_TYPE_SIZE(sfx)]) {}

typedef int8_t    XCFG_TYPE(XCFG_SFX_s08);
typedef int16_t   XCFG_TYPE(XCFG_SFX_s16);
typedef int32_t   XCFG_TYPE(XCFG_SFX_s32);
typedef int64_t   XCFG_TYPE(XCFG_SFX_s64);

typedef uint8_t   XCFG_TYPE(XCFG_SFX_u08);
typedef uint16_t  XCFG_TYPE(XCFG_SFX_u16);
typedef uint32_t  XCFG_TYPE(XCFG_SFX_u32);
typedef uint64_t  XCFG_TYPE(XCFG_SFX_u64);

typedef float     XCFG_TYPE(XCFG_SFX_f32);
typedef double    XCFG_TYPE(XCFG_SFX_f64);

typedef void *    XCFG_TYPE(XCFG_SFX_ptr);
typedef char *    XCFG_TYPE(XCFG_SFX_str);

typedef void *    XCFG_TYPE(XCFG_SFX_obj);

typedef uintptr_t xcfg_off;


/*****************************************
 * Runtime type ids for underlying types */

#define XCFG_TID(sfx) CONCATENATE(XCFG_TID_, sfx)

typedef enum {
  XCFG_TID(ANY)   = -2,
  XCFG_TID(INVAL) = -1,

#define XCFG_SFX_DO_EXPAND(sfx) \
  XCFG_TID(sfx),

  XCFG_SFX_EXPAND_all()
#undef XCFG_SFX_DO_EXPAND

  XCFG_TID(COUNT)
} xcfg_tid;

xcfg_str
xcfg_tid2sfx(xcfg_tid tid);

xcfg_str
xcfg_tid2type(xcfg_tid tid);

xcfg_tid
xcfg_sfx2tid(xcfg_str sfx);

xcfg_tid
xcfg_type2tid(xcfg_str type);


/*****************************
 * Config update management. *
 *
 * Every time a value of a structure field is changed via xcfg api a flag
 * specified in the descriptor of that field will be rased in a specific to
 * structure 'udpate' field. Same will happen to all dependent upstream structures . */

/* Specific type for 'update' field of a structure */
typedef xcfg_u64 xcfg_upd;

/* Macros used to convert flag id to flag value */
#define XCFG_UPD(id)             ((xcfg_upd)(1) << (xcfg_upd)(id))
#define XCFG_UPD_IS_SET(upd, id) ((xcfg_upd)(upd) & XCFG_UPD(id))


/* Run-Time Type Info */
typedef struct xcfg_rtti xcfg_rtti;

/* Run-Time Field Info */
typedef struct xcfg_rtfi xcfg_rtfi;

/* Main config context handle */
typedef struct xcfg xcfg;

typedef struct {
  void * (* on_create)  ();
  void   (* on_destroy) (void *cfg);
  void   (* on_update)  (void *cfg, void *udata);
} xcfg_cbs;

XCFG_EXPORT_LEAVE

#endif//__XCFG_TYPES_H__
