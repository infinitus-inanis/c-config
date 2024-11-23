#ifndef __XCFG_TREE_H__
#define __XCFG_TREE_H__

#include "xcfg-rtti.h"

/* TODO (butsuk_d): make xcfg_node and xcfg_tree opaque */

typedef struct xcfg_node xcfg_node;

/* Describes one field of a config structure. */
struct xcfg_node {
  xcfg_rtfi *rtfi; /* reference to field info */

  xcfg_str data_fld_key; /* full path to field inside config structure          */
  xcfg_u32 data_fld_off; /* full offset to field inside config structure        */
  xcfg_u32 data_upd_off; /* full offset to update field inside config structure */

  xcfg_node  *prev;  /* reference to field owner     */
  xcfg_node **next;  /* reference to children fields */
  xcfg_u32    nnext; /* children fields count        */
};

typedef struct {
  xcfg_node *node;
  xcfg_u32   depth;
  struct {
    bool bound_0 : 1;
    bool bound_N : 1;
  } meta;
} xcfg_node_tvs;

typedef xcfg_ret (* xcfg_node_tvs_visit_f)(xcfg_node_tvs *curr, xcfg_ptr context);

void
xcfg_node_destroy(xcfg_node *node);

xcfg_ptr
xcfg_node_get_fld_ptr(xcfg_node *node, xcfg_ptr data);

xcfg_ret
xcfg_node_type_check(xcfg_node *node, xcfg_tid tid);

void
xcfg_node_clear_upd(xcfg_node *node, xcfg_ptr data);

void
xcfg_node_raise_upd(xcfg_node *node, xcfg_ptr data);

xcfg_ret
xcfg_node_set_value(xcfg_node *node, xcfg_ptr data, xcfg_ptr pval);

xcfg_ret
xcfg_node_get_value(xcfg_node *node, xcfg_ptr data, xcfg_ptr pval);


typedef struct ht_ctx    ht_ctx;
typedef struct xcfg_tree xcfg_tree;
struct xcfg_tree {
  xcfg_rtti *rtti; /* main type info               */
  xcfg_rtfi  rtfi; /* proxy field info for `.rtti` */
  xcfg_node  root; /* proxy field node for `.rtti` */
  xcfg_u32   size;
  ht_ctx    *by_off;
  ht_ctx    *by_key;
};

xcfg_ret
xcfg_tree_build(xcfg_tree *tree, xcfg_rtti *rtti);

void
xcfg_tree_tvs_depth_first(xcfg_tree *tree, xcfg_node_tvs_visit_f visit, xcfg_ptr context);

void
xcfg_tree_dispose(xcfg_tree *tree);

void
xcfg_tree_dump(xcfg_tree *tree, xcfg_ptr data);

xcfg_node *
xcfg_tree_get_node_by_off(xcfg_tree *tree, xcfg_u32 off);

xcfg_node *
xcfg_tree_get_node_by_key(xcfg_tree *tree, xcfg_str key);

#endif//__XCFG_TREE_H__
