#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <list.h>
#include <stdbool.h>

struct queue {
  struct list root;
};

static inline void
queue_init(struct queue *q) {
  list_init(&q->root);
}

static inline void
queue_enqueue(struct queue *q, struct list *item) {
  list_insert_tail(item, &q->root);
}

static inline bool
queue_try_peek(struct queue *q, struct list **pitem) {
  if (!pitem)
    return false;

  if (list_empty(&q->root))
    return false;

  *pitem = list_head(&q->root);
  return true;
}

static inline struct list *
queue_peek(struct queue *q) {
  struct list *head;
  return queue_try_peek(q, &head) ? head : NULL;
}

static inline bool
queue_try_dequeue(struct queue *q, struct list **pitem) {
  struct list *head = queue_peek(q);
  if (!head)
    return false;

  if (pitem)
    *pitem = head;

  list_unlink(head);
  return true;
}

static inline struct list *
queue_dequeue(struct queue *q) {
  struct list *head;
  return queue_try_dequeue(q, &head) ? head : NULL;
}

#endif//__QUEUE_H__
