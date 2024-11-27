#ifndef __XCFG_BASE_H__
#define __XCFG_BASE_H__

#include <stddef.h>

#define MIMIC(type)                 ((type *)0)
#define FIELD_OF(type, member)      (MIMIC(type)->member)
#define FIELD_TYPE_OF(type, member)  typeof(FIELD_OF(type, member))
#define FIELD_SIZE_OF(type, member) ((size_t) sizeof(FIELD_OF(type, member)))

#define PUNE_VALUE(from, to, val)   ((union{from _; to _v;}){(val)}._v)

#ifdef offsetof
# define FIELD_OFFSET_OF(type, member) offsetof(type, member)
#else
# define FIELD_OFFSET_OF(type, member) ((uintptr_t) &FIELD_OF(type, member))
#endif

#define GET_FIELD(base, offset, type) (type)((unsigned char *)(base) + (size_t)(offset))

#define PRIM_CONCAT(s0, s1) s0##s1
#define CONCATENATE(s0, s1) PRIM_CONCAT(s0, s1)

#define PRIM_STRINGIFY(s) #s
#define STRINGIFY(s)      PRIM_STRINGIFY(s)

#define INDENTLEN        (2)
#define INDENT(str)      "%*s" str
#define INDENTARG(depth) ((depth) * INDENTLEN), ""

#define ARRAY_SIZE(arr)          (sizeof(arr) / sizeof(0[arr]))
#define ARRAY_INDEX_OF(arr, elt) ((elt - arr) / sizeof(arr))

#define DUMP_TYPE(type)  (STRINGIFY(type)), (sizeof(type))
#define DUMP_ARRAY(arr)  (arr), ARRAY_SIZE(arr)

#define in_range_exc(val, min, max) ((min) <  (val) && (val) <  (max))
#define in_range_inc(val, min, max) ((min) <= (val) && (val) <= (max))

#define for_range_exc(it, min, max) for ((it) = (min + 1); (it) < (max    ); ++(it))
#define for_range_inc(it, min, max) for ((it) = (min)    ; (it) < (max + 1); ++(it))

#ifdef __cplusplus
# define XCFG_EXPORT_ENTER export "C" {
# define XCFG_EXPORT_LEAVE }
#else
# define XCFG_EXPORT_ENTER
# define XCFG_EXPORT_LEAVE
#endif

#endif//__XCFG_BASE_H__
