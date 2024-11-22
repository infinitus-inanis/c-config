#ifndef __XCFG_TREE_H__
#define __XCFG_TREE_H__

#include "xcfg-fld.h"

/* TODO (butsuk_d): make xcfg_node and xcfg_tree opaque */

typedef struct xcfg_node xcfg_node;
struct xcfg_node {
  xcfg_str data_key;
  xcfg_u32 data_ref_off;
  xcfg_u32 data_upd_off;

  xcfg_str      key;
  xcfg_ref_desc ref;
  xcfg_fld_type type;

  xcfg_upd_desc upd;

  xcfg_node  *prev;
  xcfg_node **next;
  xcfg_u32    nnext;
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
xcfg_node_data_ref_ptr(xcfg_node *node, xcfg_ptr data);

xcfg_ret
xcfg_node_type_check(xcfg_node *node, xcfg_fld_type type);

void
xcfg_node_clear_upd(xcfg_node *node, xcfg_ptr data);

void
xcfg_node_raise_upd(xcfg_node *node, xcfg_ptr data);

xcfg_ret
xcfg_node_set_value(xcfg_node *node, xcfg_ptr data, xcfg_ptr pval);

xcfg_ret
xcfg_node_get_value(xcfg_node *node, xcfg_ptr data, xcfg_ptr pval);


typedef struct ht_ctx ht_ctx;

typedef struct xcfg_tree xcfg_tree;
struct xcfg_tree {
  xcfg_node  root;
  xcfg_u32   size;
  ht_ctx    *by_key;
  ht_ctx    *by_off;
};

xcfg_ret
xcfg_tree_build(xcfg_tree *tree,
                xcfg_str   root_key,
                xcfg_u32   root_size,
                xcfg_fld  *root_pfld,
                xcfg_u32   root_nfld);

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
