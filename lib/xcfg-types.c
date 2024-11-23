#include "xcfg-types.h"

static xcfg_str lt_tid2sfx[XCFG_TID_COUNT] = {
#define XCFG_SFX_DO_EXPAND(sfx) \
  [XCFG_TID(sfx)] = #sfx,

  XCFG_SFX_EXPAND_all()
#undef XCFG_SFX_DO_EXPAND
};

xcfg_str
xcfg_tid2sfx(xcfg_tid tid)
{
  if (tid >= XCFG_TID_COUNT)
    return NULL;

  return lt_tid2sfx[tid];
}