#include "xcfg-impl.h"
#include "xcfg-data.h"

xcfg_ptr
xcfg_get_data(xcfg *cfg)
{
  return cfg->data;
}

void
xcfg_lock_data(xcfg *cfg)
{
  pthread_mutex_lock(&cfg->data_mutex);
}

void
xcfg_unlock_data(xcfg *cfg)
{
  pthread_mutex_unlock(&cfg->data_mutex);
}

static xcfg_node *
get_node_by_key(xcfg *cfg, xcfg_key *key, xcfg_tid tid)
{
  xcfg_node *node = NULL;

  switch (key->type) {
    case XCFG_KEY_OFF: {
      node = xcfg_tree_get_node_by_off(&cfg->tree, key->off);
    } break;
    case XCFG_KEY_STR: {
      node = xcfg_tree_get_node_by_str(&cfg->tree, key->str);
    } break;
  }
  if (!node)
    return NULL;

  if (tid != XCFG_TID_ANY && node->rtfi->tid != tid)
    return NULL;

  return node;
}

static xcfg_ret
do_set(xcfg *cfg, xcfg_node *node, xcfg_ptr pval)
{
  xcfg_ret xret;

  xcfg_lock_data(cfg);
  xret = xcfg_node_set_value(node, xcfg_get_data(cfg), pval);
  xcfg_unlock_data(cfg);

  return xret;
}

static xcfg_ret
do_get(xcfg *cfg, xcfg_node *node, xcfg_ptr pval)
{
  xcfg_ret xret;

  xcfg_lock_data(cfg);
  xret = xcfg_node_get_value(node, xcfg_get_data(cfg), pval);
  xcfg_unlock_data(cfg);

  return xret;
}

static xcfg_ret
do_set_by_key(xcfg *cfg, xcfg_key *key, xcfg_tid tid, xcfg_ptr pval)
{
  xcfg_node *node;

  if (!cfg)
    return XCFG_RET_INVALID;

  node = get_node_by_key(cfg, key, tid);
  if (!node)
    return XCFG_RET_UNKNOWN;

  return do_set(cfg, node, pval);
}

static xcfg_ret
do_get_by_key(xcfg *cfg, xcfg_key *key, xcfg_tid tid, xcfg_ptr pval)
{
  xcfg_node *node;

  if (!cfg)
    return XCFG_RET_INVALID;

  node = get_node_by_key(cfg, key, tid);
  if (!node)
    return XCFG_RET_UNKNOWN;

  return do_get(cfg, node, pval);
}

#define XCFG_SFX_DO_EXPAND(sfx) \
  xcfg_ret \
  XCFG_SET(sfx)(xcfg *cfg, xcfg_key key, XCFG_TYPE(sfx) val) \
  { return do_set_by_key(cfg, &key, XCFG_TID(sfx), (xcfg_ptr)(&val)); }

  XCFG_SFX_EXPAND_all()
#undef XCFG_SFX_DO_EXPAND

#define XCFG_SFX_DO_EXPAND(sfx) \
  xcfg_ret \
  XCFG_GET(sfx)(xcfg *cfg, xcfg_key key, XCFG_TYPE(sfx) *pval) \
  { return do_get_by_key(cfg, &key, XCFG_TID(sfx), (xcfg_ptr)(pval)); }

  XCFG_SFX_EXPAND_val()
  XCFG_SFX_EXPAND_str()
  /* getter for `XCFG_SFX_ptr` is a spacial case */
  /* getter for `XCFG_SFX_obj` is a special case */
#undef XCFG_SFX_DO_EXPAND

xcfg_ret
XCFG_GET(XCFG_SFX_ptr)(xcfg *cfg, xcfg_key key, xcfg_ptr pptr)
{ return do_get_by_key(cfg, &key, XCFG_TID(XCFG_SFX_ptr), (xcfg_ptr)(pptr)); }

xcfg_ret
XCFG_GET(XCFG_SFX_obj)(xcfg *cfg, xcfg_key key, xcfg_obj obj)
{ return do_get_by_key(cfg, &key, XCFG_TID(XCFG_SFX_obj), (xcfg_ptr)(obj)); }