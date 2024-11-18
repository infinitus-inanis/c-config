#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include <stdint.h>
#include <stdbool.h>

/* WARNING (butsuk_d):
    It is VERY simple hashtable that must be used VERY carefuly:
      - Hash collisions resolved via 'linear probing'

      - Two types of keys:
        1) HT_KEY_INT:
          unsigned integer value of size 64bit

        2) HT_KEY_STR:
          C string value of any length, assumed to be null-terminated

    ATTENTION:
      string keys aren't copied so..
      they MUST live longer then last use of hashtable */

typedef uint64_t ht_u64;
typedef char *   ht_str;

typedef enum {
  HT_KEY_INT,
  HT_KEY_STR,
} ht_key_type;

typedef union {
  ht_u64 _int;
  ht_str _str;
} ht_key;

#define ht_key_int(v) (ht_key) { ._int = (ht_u64)(v) }
#define ht_key_str(v) (ht_key) { ._str = (ht_str)(v) }


typedef struct ht_ctx ht_ctx;

ht_ctx *
ht_create(ht_key_type key_type, ht_u64 cap);

void
ht_destroy(ht_ctx *ht);

bool
ht_set(ht_ctx *ht, ht_key key, void *val);

void *
ht_get(ht_ctx *ht, ht_key key);

ht_u64
ht_length(ht_ctx *ht);

#endif//__HASHTABLE_H__
