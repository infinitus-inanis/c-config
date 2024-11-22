#ifndef __XCFG_BASE_H__
#define __XCFG_BASE_H__

#include <stddef.h>

#define FIELD_OF(type, member)        ((type *)0)->member
#define FIELD_SIZE_OF(type, member)   ((size_t) sizeof(FIELD_OF(type, member)))
#define FIELD_OFFSET_OF(type, member) ((size_t) & FIELD_OF(type, member))

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

#endif//__XCFG_BASE_H__
