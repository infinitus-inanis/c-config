#ifndef __LIST_H__
#define __LIST_H__

#include <stddef.h>

struct list {
  struct list *next;
  struct list *prev;
};

#define list_root(_self) (struct list) { (_self), (_self) }

static inline void
list_init(struct list *root) {
  root->next = root;
  root->prev = root;
}

static inline void
list_link(struct list *prev, struct list *next) {
  next->prev = prev;
  prev->next = next;
}

static inline void
list_unlink(struct list *node) {
  list_link(node->prev, node->next);
}

static inline int
list_empty(struct list *root) {
  return !root->next || root->next == root;
}

static inline void
list_insert(struct list *node, struct list *prev, struct list *next) {
  if (list_empty(node)) {
    list_link(node, next);
    list_link(prev, node);
  } else {
    list_link(node->prev, next);
    list_link(prev, node);
  }
}

static inline void
list_insert_head(struct list *node, struct list *root) {
  list_insert(node, root, root->next);
}

static inline void
list_insert_tail(struct list *node, struct list *root) {
  list_insert(node, root->prev, root);
}

#define list_head(_root) (_root)->next
#define list_tail(_root) (_root)->prev

#define list_for_each(_node, _root) \
  for (_node  = (_root)->next; \
       _node != (_root); \
       _node  = (_node)->next)

#define list_data(_node, _type, _member) \
  (_type *)((char *)(_node) - offsetof(_type, _member))

#define list_head_data(_root, _type, _member) \
  list_data(list_head(_root), _type, _member)

#define list_tail_data(_root, _type, _member) \
  list_data(list_tail(_root), _type, _member)

#define list_next_data(_data, _member) \
  list_data((_data)->_member.next, typeof(*(_data)), _member)

#define list_prev_data(_data, _member) \
  list_data((_data)->_member.prev, typeof(*(_data)), _member)

#define list_for_each_data(_data, _root, _member) \
  for (_data = list_head_data(_root, typeof(*_data), _member); \
      &_data->_member != (_root); \
       _data = list_next_data(_data, _member))

#define list_for_each_data_safe(_data, _temp, _root, _member) \
  for (_data = list_head_data(_root, typeof(*_data), _member), \
       _temp = list_next_data(_data, _member); \
      &_data->_member != (_root); \
       _data = _temp, \
       _temp = list_next_data(_temp, _member))

#endif//__LIST_H__
