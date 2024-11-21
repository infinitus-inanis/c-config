#ifndef __XCFG_TYPES_H__
#define __XCFG_TYPES_H__

#include "xcfg-base.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
  XCFG_RET_FAILRUE = -1,
  XCFG_RET_SUCCESS =  0,

  XCFG_RET_INVALID = XCFG_RET_FAILRUE - 1,
  XCFG_RET_UNKNOWN = XCFG_RET_FAILRUE - 2,
} xcfg_ret;


/******************************** *
 * Suffixes for underlying types. *
 * Basicaly are a unique type indentifiers. */

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

#define XCFG_SFX_EXPAND_VAL() \
  XCFG_SFX_DO_EXPAND(XCFG_SFX_s08) \
  XCFG_SFX_DO_EXPAND(XCFG_SFX_s16) \
  XCFG_SFX_DO_EXPAND(XCFG_SFX_s32) \
  XCFG_SFX_DO_EXPAND(XCFG_SFX_s64) \
  XCFG_SFX_DO_EXPAND(XCFG_SFX_u08) \
  XCFG_SFX_DO_EXPAND(XCFG_SFX_u16) \
  XCFG_SFX_DO_EXPAND(XCFG_SFX_u32) \
  XCFG_SFX_DO_EXPAND(XCFG_SFX_u64) \
  XCFG_SFX_DO_EXPAND(XCFG_SFX_f32) \
  XCFG_SFX_DO_EXPAND(XCFG_SFX_f64)

#define XCFG_SFX_EXPAND_PTR() \
  XCFG_SFX_DO_EXPAND(XCFG_SFX_ptr)

#define XCFG_SFX_EXPAND_STR() \
  XCFG_SFX_DO_EXPAND(XCFG_SFX_str)

#define XCFG_SFX_EXPAND_OBJ() \
  XCFG_SFX_DO_EXPAND(XCFG_SFX_obj)

#define XCFG_SFX_EXPAND_ALL() \
  XCFG_SFX_EXPAND_VAL() \
  XCFG_SFX_EXPAND_PTR() \
  XCFG_SFX_EXPAND_STR() \
  XCFG_SFX_EXPAND_OBJ()

/******************************************
 * Type definitions for underlying types. */

/* Resolves type suffix to type definition */
#define XCFG_TYPE(sfx) CONCATENATE(xcfg_, sfx)

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


/*****************************
 * Config update management. *
 *
 * Every time a value of a structure field is changed via xcfg api a flag
 * specified in the descriptor of that field will be rased in a specific to
 * structure 'udpate' field. Same will happen to all dependent upstream structures . */

/* Specific type for 'update' field of a structure */
typedef xcfg_u64 xcfg_upd;

/* Macros used to convert flag id to flag value */
#define XCFG_UPD(id) ((xcfg_upd)(1) << (xcfg_upd)(id))


/* Forward delcare field descriptor type because
    we want api to be as clean as possible */
typedef struct xcfg_fld xcfg_fld;

/* Forward declare main config context type */
typedef struct xcfg xcfg;

#endif//__XCFG_TYPES_H__
