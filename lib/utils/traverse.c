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
  tvs_node  *stack, *temp, curr;
  uint32_t   point, nnext, i;
  void     **next;

  if (nroot == 0)
    return;

  stack = calloc(nall, sizeof *stack);
  point = 0;

  /* root nodes will probably come in natural order
      stack logic will ruin it, so push them in reverse */

  for (i = nroot; i > 0; --i) {
    temp = &stack[point++];
    temp->data = proot[i - 1];
    temp->depth = 0;
    temp->meta.next_0 = i == 0;
    temp->meta.next_N = i == nroot;
  }

  while (point) {
    curr  = stack[--point];
    next  = NULL;
    nnext = 0;

    if (!visit(context, &curr))
      continue;

    if (!populate(context, &curr, &next, &nnext))
      continue;

    /* sanity check */
    if (!nnext || !next)
      continue;

    /* next nodes will probably come in natural order
        stack logic will ruin it, so push them in reverse */
    for (i = nnext; i > 0; --i) {
      temp = &stack[point++];
      temp->data  = next[i - 1];
      temp->depth = curr.depth + 1;
      temp->meta.next_0 = i == 0;
      temp->meta.next_N = i == nnext;
    }
  }

  free(stack);
}
