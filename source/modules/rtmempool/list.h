/*****************************************************************************
 *
 *   Linux kernel header adapted for user-mode
 *   The 2.6.17-rt1 version was used.
 *
 *   Original copyright holders of this code are unknown, they were not
 *   mentioned in the original file.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *****************************************************************************/

#ifndef LINUX_LIST_H_INCLUDED
#define LINUX_LIST_H_INCLUDED

/* This file is from Linux Kernel (include/linux/list.h)
 * and modified by removing hardware prefetching of list items
 * and added list_split_tail* functions.
 *
 * Here by copyright, credits attributed to wherever they belong.
 * Filipe Coelho (aka falkTX <falktx@falktx.com>)
 */

#ifdef __cplusplus
# include <cstddef>
#else
# include <stddef.h>
#endif

#ifndef offsetof
# define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

/**
 * container_of - cast a member of a structure out to the containing structure
 * @param ptr:    the pointer to the member.
 * @param type:   the type of the container struct this is embedded in.
 * @param member  the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({                  \
    typeof( ((type *)0)->member ) *__mptr = (ptr);          \
    (type *)( (char *)__mptr - offsetof(type, member) );})

#define container_of_const(ptr, type, member) ({            \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (const type *)( (const char *)__mptr - offsetof(type, member) );})

#define prefetch(x) (x = x)

/*
 * These are non-NULL pointers that will result in page faults
 * under normal circumstances, used to verify that nobody uses
 * non-initialized list entries.
 */
#define LIST_POISON1  ((void *) 0x00100100)
#define LIST_POISON2  ((void *) 0x00200200)

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

struct list_head {
    struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add(struct list_head *new_, struct list_head *prev, struct list_head *next)
{
    next->prev = new_;
    new_->next = next;
    new_->prev = prev;
    prev->next = new_;
}

/**
 * list_add - add a new entry
 * @param new_  new entry to be added
 * @param head  list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void list_add(struct list_head *new_, struct list_head *head)
{
    __list_add(new_, head, head->next);
}

/**
 * list_add_tail - add a new entry
 * @param new_  new entry to be added
 * @param head  list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void list_add_tail(struct list_head *new_, struct list_head *head)
{
    __list_add(new_, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct list_head *prev, struct list_head *next)
{
    next->prev = prev;
    prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @param entry  the element to delete from the list.
 * Note: list_empty on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    entry->next = (struct list_head*)LIST_POISON1;
    entry->prev = (struct list_head*)LIST_POISON2;
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @param entry  the element to delete from the list.
 */
static inline void list_del_init(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    INIT_LIST_HEAD(entry);
}

/**
 * list_move - delete from one list and add as another's head
 * @param list  the entry to move
 * @param head  the head that will precede our entry
 */
static inline void list_move(struct list_head *list, struct list_head *head)
{
    __list_del(list->prev, list->next);
    list_add(list, head);
}

/**
 * list_move_tail - delete from one list and add as another's tail
 * @param list  the entry to move
 * @param head  the head that will follow our entry
 */
static inline void list_move_tail(struct list_head *list, struct list_head *head)
{
    __list_del(list->prev, list->next);
    list_add_tail(list, head);
}

/**
 * list_empty - tests whether a list is empty
 * @param head the list to test.
 */
static inline int list_empty(const struct list_head *head)
{
    return head->next == head;
}

/**
 * list_empty_careful - tests whether a list is
 * empty _and_ checks that no other CPU might be
 * in the process of still modifying either member
 *
 * NOTE: using list_empty_careful() without synchronization
 * can only be safe if the only activity that can happen
 * to the list entry is list_del_init(). Eg. it cannot be used
 * if another CPU could re-list_add() it.
 *
 * @param head  the list to test.
 */
static inline int list_empty_careful(const struct list_head *head)
{
    struct list_head *next = head->next;
    return (next == head) && (next == head->prev);
}

static inline void __list_splice(struct list_head *list, struct list_head *head)
{
    struct list_head *first = list->next;
    struct list_head *last = list->prev;
    struct list_head *at = head->next;

    first->prev = head;
    head->next = first;

    last->next = at;
    at->prev = last;
}

static inline void __list_splice_tail(struct list_head *list, struct list_head *head)
{
    struct list_head *first = list->next;
    struct list_head *last = list->prev;
    struct list_head *at = head->prev;

    first->prev = at;
    at->next = first;

    last->next = head;
    head->prev = last;
}

/**
 * list_splice - join two lists
 * @param list  the new list to add.
 * @param head  the place to add it in the first list.
 */
static inline void list_splice(struct list_head *list, struct list_head *head)
{
    if (!list_empty(list))
        __list_splice(list, head);
}

/**
 * list_splice_tail - join two lists
 * @param list  the new list to add.
 * @param head  the place to add it in the first list.
 *
 * @a list goes to the end (at head->prev)
 */
static inline void list_splice_tail(struct list_head *list, struct list_head *head)
{
    if (!list_empty(list))
        __list_splice_tail(list, head);
}

/**
 * list_splice_init - join two lists and reinitialise the emptied list.
 * @param list  the new list to add.
 * @param head  the place to add it in the first list.
 *
 * The list at @a list is reinitialised
 */
static inline void list_splice_init(struct list_head *list, struct list_head *head)
{
    if (!list_empty(list)) {
        __list_splice(list, head);
        INIT_LIST_HEAD(list);
    }
}

/**
 * list_splice_tail_init - join two lists and reinitialise the emptied list.
 * @param list  the new list to add.
 * @param head  the place to add it in the first list.
 *
 * The list @a list is reinitialised
 * @a list goes to the end (at head->prev)
 */
static inline void list_splice_tail_init(struct list_head *list, struct list_head *head)
{
    if (!list_empty(list)) {
        __list_splice_tail(list, head);
        INIT_LIST_HEAD(list);
    }
}

/**
 * list_entry - get the struct for this entry
 * @param ptr:    the &struct list_head pointer.
 * @param type:   the type of the struct this is embedded in.
 * @param member  the name of the list_struct within the struct.
 */
#if (defined(__GNUC__) || defined(__clang__)) && ! defined(__STRICT_ANSI__)
# define list_entry(ptr, type, member) \
    container_of(ptr, type, member)
# define list_entry_const(ptr, type, member) \
    container_of_const(ptr, type, member)
#else
# define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr)-offsetof(type, member)))
# define list_entry_const(ptr, type, member) \
    ((const type *)((const char *)(ptr)-offsetof(type, member)))
#endif

/**
 * list_for_each - iterate over a list
 * @param pos   the &struct list_head to use as a loop counter.
 * @param head  the head for your list.
 */
#define list_for_each(pos, head) \
    for (pos = (head)->next; prefetch(pos->next), pos != (head); \
    pos = pos->next)

/**
 * __list_for_each  - iterate over a list
 * @param pos   the &struct list_head to use as a loop counter.
 * @param head  the head for your list.
 *
 * This variant differs from list_for_each() in that it's the
 * simplest possible list iteration code, no prefetching is done.
 * Use this for code that knows the list to be very short (empty
 * or 1 entry) most of the time.
 */
#define __list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_prev - iterate over a list backwards
 * @param pos   the &struct list_head to use as a loop counter.
 * @param head  the head for your list.
 */
#define list_for_each_prev(pos, head) \
    for (pos = (head)->prev; prefetch(pos->prev), pos != (head); \
    pos = pos->prev)

/**
 * list_for_each_safe - iterate over a list safe against removal of list entry
 * @param pos   the &struct list_head to use as a loop counter.
 * @param n     another &struct list_head to use as temporary storage
 * @param head  the head for your list.
 */
#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
    pos = n, n = pos->next)

/**
 * list_for_each_entry - iterate over list of given type
 * @param pos     the type * to use as a loop counter.
 * @param head    the head for your list.
 * @param member  the name of the list_struct within the struct.
 */
#define list_for_each_entry(pos, head, member)        \
    for (pos = list_entry((head)->next, typeof(*pos), member);  \
    prefetch(pos->member.next), &pos->member != (head);  \
    pos = list_entry(pos->member.next, typeof(*pos), member))

/**
 * list_for_each_entry_reverse - iterate backwards over list of given type.
 * @param pos     the type * to use as a loop counter.
 * @param head    the head for your list.
 * @param member  the name of the list_struct within the struct.
 */
#define list_for_each_entry_reverse(pos, head, member)      \
    for (pos = list_entry((head)->prev, typeof(*pos), member);  \
    prefetch(pos->member.prev), &pos->member != (head);  \
    pos = list_entry(pos->member.prev, typeof(*pos), member))

/**
 * list_prepare_entry - prepare a pos entry for use as a start point in list_for_each_entry_continue
 * @param pos     the type * to use as a start point
 * @param head    the head of the list
 * @param member  the name of the list_struct within the struct.
 */
#define list_prepare_entry(pos, head, member) \
    ((pos) ? : list_entry(head, typeof(*pos), member))

/**
 * list_for_each_entry_continue - iterate over list of given type continuing after existing point
 * @param pos     the type * to use as a loop counter.
 * @param head    the head for your list.
 * @param member  the name of the list_struct within the struct.
 */
#define list_for_each_entry_continue(pos, head, member)     \
    for (pos = list_entry(pos->member.next, typeof(*pos), member);  \
    prefetch(pos->member.next), &pos->member != (head);  \
    pos = list_entry(pos->member.next, typeof(*pos), member))

/**
 * list_for_each_entry_from - iterate over list of given type continuing from existing point
 * @param pos     the type * to use as a loop counter.
 * @param head    the head for your list.
 * @param member  the name of the list_struct within the struct.
 */
#define list_for_each_entry_from(pos, head, member)       \
    for (; prefetch(pos->member.next), &pos->member != (head);  \
    pos = list_entry(pos->member.next, typeof(*pos), member))

/**
 * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @param pos     the type * to use as a loop counter.
 * @param n       another type * to use as temporary storage
 * @param head    the head for your list.
 * @param member  the name of the list_struct within the struct.
 */
#define list_for_each_entry_safe(pos, n, head, member)      \
    for (pos = list_entry((head)->next, typeof(*pos), member),  \
    n = list_entry(pos->member.next, typeof(*pos), member); \
    &pos->member != (head);          \
    pos = n, n = list_entry(n->member.next, typeof(*n), member))

/**
 * list_for_each_entry_safe_continue -  iterate over list of given type continuing after existing point safe against removal of list entry
 * @param pos     the type * to use as a loop counter.
 * @param n       another type * to use as temporary storage
 * @param head    the head for your list.
 * @param member  the name of the list_struct within the struct.
 */
#define list_for_each_entry_safe_continue(pos, n, head, member)     \
    for (pos = list_entry(pos->member.next, typeof(*pos), member),    \
    n = list_entry(pos->member.next, typeof(*pos), member);   \
    &pos->member != (head);            \
    pos = n, n = list_entry(n->member.next, typeof(*n), member))

/**
 * list_for_each_entry_safe_from - iterate over list of given type from existing point safe against removal of list entry
 * @param pos     the type * to use as a loop counter.
 * @param n       another type * to use as temporary storage
 * @param head    the head for your list.
 * @param member  the name of the list_struct within the struct.
 */
#define list_for_each_entry_safe_from(pos, n, head, member)       \
    for (n = list_entry(pos->member.next, typeof(*pos), member);    \
    &pos->member != (head);            \
    pos = n, n = list_entry(n->member.next, typeof(*n), member))

/**
 * list_for_each_entry_safe_reverse - iterate backwards over list of given type safe against removal of list entry
 * @param pos     the type * to use as a loop counter.
 * @param n       another type * to use as temporary storage
 * @param head    the head for your list.
 * @param member  the name of the list_struct within the struct.
 */
#define list_for_each_entry_safe_reverse(pos, n, head, member)    \
    for (pos = list_entry((head)->prev, typeof(*pos), member),  \
    n = list_entry(pos->member.prev, typeof(*pos), member); \
    &pos->member != (head);          \
    pos = n, n = list_entry(n->member.prev, typeof(*n), member))

#endif // LINUX_LIST_H_INCLUDED
