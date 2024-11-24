#ifndef __XCFG_BASE_H__
#define __XCFG_BASE_H__

#include <stddef.h>

#define MIMIC(type)                   ((type *)0)
#define FIELD_OF(type, member)        (MIMIC(type)->member)
#define FIELD_SIZE_OF(type, member)   ((size_t) sizeof(FIELD_OF(type, member)))
#define FIELD_OFFSET_OF(type, member) ((size_t) & FIELD_OF(type, member))

#define GET_FIELD(base, offset, type) (type)((unsigned char *)(base) + (size_t)(offset))

#define PRIM_CONCAT(s0, s1) s0##s1
#define CONCATENATE(s0, s1) PRIM_CONCAT(s0, s1)

#define PRIM_STRINGIFY(s) #s
#define STRINGIFY(s)      PRIM_STRINGIFY(s)

#define INDENTLEN        (2)
#define INDENT(str)      "%*s" str
#define INDENTARG(depth) ((depth) * INDENTLEN), ""

#define ARRAY_SIZE(arr)  (sizeof(arr) / sizeof(0[arr]))

#define DUMP_TYPE(type)  (STRINGIFY(type)), (sizeof(type))
#define DUMP_ARRAY(arr)  (arr), ARRAY_SIZE(arr)

#ifdef __cplusplus
# define XCFG_EXPORT_ENTER export "C" {
# define XCFG_EXPORT_LEAVE }
#else
# define XCFG_EXPORT_ENTER
# define XCFG_EXPORT_LEAVE
#endif

#endif//__XCFG_BASE_H__
