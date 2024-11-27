#ifndef __TRAVERSE_H__
#define __TRAVERSE_H__

#include <stdint.h>
#include <stdbool.h>

typedef struct tvs_node {
  void     *data;
  uint32_t  depth;

  struct {
    bool next_0 : 1;
    bool next_N : 1;
  } meta;

} tvs_node;

typedef bool (* tvs_visit_f)    (void *context, tvs_node *curr);
typedef bool (* tvs_populate_f) (void *context, tvs_node *curr, void ***pnext, uint32_t *nnext);

void
tvs_depth_first(void          *context,
                void         **proot,
                uint32_t       nroot,
                uint32_t       nall,
                tvs_visit_f    visit,
                tvs_populate_f populate);


#endif//__TRAVERSE_H__
