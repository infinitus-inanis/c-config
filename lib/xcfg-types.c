#include "xcfg-types.h"
#include "utils/hashtable.h"

#include <memory.h>

typedef struct {
  xcfg_str sfx;
  xcfg_str type;
} xcfg_tid_info;

#define XCFG_TID_INFO(sfx) { STRINGIFY(sfx), STRINGIFY(XCFG_TYPE(sfx)) }

static xcfg_tid_info tid2info[XCFG_TID_COUNT] = {
#define XCFG_SFX_DO_EXPAND(sfx) \
  [XCFG_TID(sfx)] = XCFG_TID_INFO(sfx),

  XCFG_SFX_EXPAND_all()
#undef XCFG_SFX_DO_EXPAND
};

static ht_ctx *by_sfx;
static ht_ctx *by_type;

static void ht_ctor() __attribute__((constructor));
static void ht_ctor()
{
  xcfg_tid tid;

  by_sfx  = ht_create(HT_KEY_STR, XCFG_TID_COUNT);
  by_type = ht_create(HT_KEY_STR, XCFG_TID_COUNT);

  for_range_exc(tid, XCFG_TID_INVAL, XCFG_TID_COUNT) {
    xcfg_tid_info *info = &tid2info[tid];
    ht_set(by_sfx,  ht_key_str(info->sfx),  info);
    ht_set(by_type, ht_key_str(info->type), info);
  }
}

static void ht_dtor() __attribute__((destructor));
static void ht_dtor()
{
  ht_destroy(by_sfx);
  ht_destroy(by_type);
}

xcfg_str
xcfg_tid2sfx(xcfg_tid tid)
{
  if (!in_range_exc(tid, XCFG_TID_INVAL, XCFG_TID_COUNT))
    return NULL;

  return tid2info[tid].sfx;
}

xcfg_str
xcfg_tid2type(xcfg_tid tid)
{
  if (!in_range_exc(tid, XCFG_TID_INVAL, XCFG_TID_COUNT))
    return NULL;

  return tid2info[tid].type;
}

xcfg_tid
xcfg_sfx2tid(xcfg_str sfx)
{
  xcfg_tid_info *info = ht_get(by_sfx, ht_key_str(sfx));
  if (!info)
    return XCFG_TID_INVAL;

  return ARRAY_INDEX_OF(tid2info, info);
}

xcfg_tid
xcfg_type2tid(xcfg_str type)
{
  xcfg_tid_info *info = ht_get(by_type, ht_key_str(type));
  if (!info)
    return XCFG_TID_INVAL;

  return ARRAY_INDEX_OF(tid2info, info);
}
