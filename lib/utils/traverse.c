#include "traverse.h"

#include <stdlib.h>

void
tvs_depth_first(void          *context,
                void         **proot,
                uint32_t       nroot,
                uint32_t       nall,
                tvs_visit_f    visit,
                tvs_populate_f populate)
{
  tvs_node  *stack;
  int64_t    point;
  tvs_node   curr;
  tvs_node  *temp;
  void     **next;
  uint32_t   nnext;
  uint32_t   i;

  if (nroot == 0)
    return;

  stack = calloc(nall, sizeof *stack);
  point = -1;

  /* root nodes will probably come in natural order
      stack logic will ruin it, so push them in reverse */

  for (i = nroot; i > 0; --i) {
    temp = &stack[++point];
    temp->data = proot[i - 1];
    temp->depth = 0;
    temp->meta.next_0 = i == 0;
    temp->meta.next_N = i == nroot;
  }

  while (point >= 0) {
    curr = stack[point--];

    if (!visit(context, &curr))
      continue;

    if (!populate(context, &curr, &next, &nnext))
      continue;

    if (!nnext)
      continue;

    /* next nodes will probably come in natural order
        stack logic will ruin it, so push them in reverse */
    for (i = nnext; i > 0; --i) {
      temp = &stack[++point];
      temp->data  = next[i - 1];
      temp->depth = curr.depth + 1;
      temp->meta.next_0 = i == 0;
      temp->meta.next_N = i == nnext;
    }

    nnext = 0;
  }

  free(stack);
}