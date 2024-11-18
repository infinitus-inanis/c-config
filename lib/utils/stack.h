#ifndef __STACK_H__
#define __STACK_H__

#include "list.h"
#include <stdbool.h>

struct stack {
  struct list root;
};

static inline void
stack_init(struct stack *s) {
  list_init(&s->root);
}

static inline void
stack_push(struct stack *s, struct list *item) {
  list_insert_head(item, &s->root);
}

static inline bool
stack_try_peek(struct stack *s, struct list **pitem) {
  if (!pitem)
    return false;

  if (list_empty(&s->root))
    return false;

  *pitem = list_head(&s->root);
  return true;
}

static inline struct list *
stack_peek(struct stack *s) {
  struct list *head;
  return stack_try_peek(s, &head) ? head : NULL;
}

static inline bool
stack_try_pop(struct stack *s, struct list **pitem) {
  struct list *head = stack_peek(s);
  if (!head)
    return;

  if (pitem)
    *pitem = head;

  list_unlink(head);
  return true;
}

static inline struct list *
stack_pop(struct stack *s) {
  struct list *head;
  return stack_try_pop(s, &head) ? head : NULL;
}


#endif//__STACK_H__
