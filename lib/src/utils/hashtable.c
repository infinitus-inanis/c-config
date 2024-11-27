#include "hashtable.h"

#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>

#define HT_CAP_GROW_INIT 16

#define ht_get_idx(h, cap)  ((h)  & (cap - 1))
#define ht_get_ent(i, arr) &((i)[arr])
#define ht_adv_idx(i, cap)  ((i) != (cap - 1) ? (i + 1) : (0))

typedef uint8_t ht_u08;

typedef struct {
  ht_key  key;
  void   *val;
} ht_ent;

typedef ht_u64 (* ht_hash_f)  (ht_key key);
typedef bool   (* ht_equal_f) (ht_key lhs, ht_key rhs);

typedef struct {
  ht_hash_f  hash;
  ht_equal_f equal;
} ht_ops;

struct ht_ctx {
  ht_ops *ops;
  ht_ent *arr;
  ht_u64  cap;
  ht_u64  len;
};

static void
ht_set_impl(ht_ops *ops, ht_ent *arr, ht_u64 cap, ht_u64 *len, ht_key key, void *val)
{
  ht_u64  h = ops->hash(key);
  ht_u64  i = ht_get_idx(h, cap);
  ht_ent *e = ht_get_ent(i, arr);

  while (e->val) {
    if (ops->equal(e->key, key)) {
      e->val = val;
      return;
    }

    i = ht_adv_idx(i, cap);
    e = ht_get_ent(i, arr);
  }

  e->key = key;
  e->val = val;

  if (len)
    *len += 1;
}

static bool
ht_grow(ht_ctx *ht)
{
  ht_ent *new_arr;
  ht_u64  new_cap;
  ht_u64  i;

  /* reached maximum capacity */
  if (ht->cap == UINT64_MAX)
    return false;

  new_cap = ht->cap ? ht->cap * 2 : HT_CAP_GROW_INIT;

  /* correct overflow to reach maximum */
  if (new_cap < ht->cap)
    new_cap = UINT64_MAX;

  new_arr = calloc(new_cap, sizeof *new_arr);
  if (!new_arr)
    return false;

  for (i = 0; i < ht->cap; ++i) {
    ht_ent *e = &ht->arr[i];
    if (e->val)
      ht_set_impl(ht->ops, new_arr, new_cap, NULL, e->key, e->val);
  }

  free(ht->arr);
  ht->arr = new_arr;
  ht->cap = new_cap;

  return true;
}

/* 64bit fnv-1a hash function */
#define fnv_basis 0xcbf29ce484222325
#define fnv_prime 0x00000100000001b3

static ht_u64
ht_fnv_hash_64(ht_u08 *arr, ht_u64 len)
{
  ht_u64 h = (ht_u64)(fnv_basis);
  ht_u08 i;

  for (i = 0; i < len; ++i) {
    h *= (ht_u64)(fnv_prime);
    h ^= (ht_u64)(arr[i]);
  }

  return h;
}

static inline ht_u64
ht_int_hash(ht_key key)
{
  return ht_fnv_hash_64((ht_u08 *)(&key._int), sizeof key._int);
}

static inline bool
ht_int_equal(ht_key lhs, ht_key rhs)
{
  return lhs._int == rhs._int;
}

static ht_ops ht_int_ops = { ht_int_hash, ht_int_equal };

/* uses fnv hash function */
static inline ht_u64
ht_str_hash(ht_key key)
{
  return ht_fnv_hash_64((ht_u08 *)(key._str), strlen(key._str));
}

static inline bool
ht_str_equal(ht_key lhs, ht_key rhs)
{
  return !strcmp(lhs._str, rhs._str);
}

static ht_ops ht_str_ops = { ht_str_hash, ht_str_equal };

ht_ctx *
ht_create(ht_key_type key_type, ht_u64 cap)
{
  ht_ctx *ctx;

  ctx = calloc(1, sizeof *ctx);
  if (!ctx)
    return NULL;

  switch (key_type) {
  case HT_KEY_INT:
    ctx->ops = &ht_int_ops;
    break;

  case HT_KEY_STR:
    ctx->ops = &ht_str_ops;
    break;
  }
  assert(ctx->ops);

  ctx->cap = cap;
  if (!!ctx->cap)
    ctx->arr = calloc(ctx->cap, sizeof *ctx->arr);

  return ctx;
}

void
ht_destroy(ht_ctx *ht)
{
  if (!ht)
    return;

  free(ht->arr);
  free(ht);
}

bool
ht_set(ht_ctx *ht, ht_key key, void *val)
{
  assert(ht);
  assert(val);

  if (ht->len >= ht->cap / 2)
    if (!ht_grow(ht))
      return false;

  ht_set_impl(ht->ops, ht->arr, ht->cap, &ht->len, key, val);
  return true;
}

void *
ht_get(ht_ctx *ht, ht_key key)
{
  ht_u64  h = ht->ops->hash(key);
  ht_u64  i = ht_get_idx(h, ht->cap);
  ht_ent *e = ht_get_ent(i, ht->arr);

  while (e->val) {
    if (ht->ops->equal(e->key, key))
      return e->val;

    i = ht_adv_idx(i, ht->cap);
    e = ht_get_ent(i, ht->arr);
  }
  return NULL;
}

ht_u64
ht_length(ht_ctx *ht)
{
  return ht->len;
}