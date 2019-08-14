// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * ktf_map.h: simple objects with key lookup to be inlined into a
 *    larger object
 */

#ifndef _KTF_MAP_H
#define _KTF_MAP_H
#include <linux/kref.h>
#include <linux/version.h>
#include <linux/rbtree.h>

#define	KTF_MAX_KEY 64
#define KTF_MAX_NAME (KTF_MAX_KEY - 1)

struct ktf_map_elem;

/* Compare function called to compare element keys - optional and if
 * not specified we revert to string comparison.  Should return < 0
 * if first key < second, > 0 if first key > second, and 0 if they are
 * identical.
 */
typedef int (*ktf_map_elem_comparefn)(const char *, const char *);

/* A convenience unsigned int compare function as an alternative
 * to the string compare:
 */
int ktf_uint_compare(const char *a, const char *b);

/* Free function called when elem refcnt is 0 - optional and of course for
 * dynamically-allocated elems only.
 */
typedef void (*ktf_map_elem_freefn)(struct ktf_map_elem *);

struct ktf_map {
	struct rb_root root; /* The rb tree holding the map */
	size_t size;	     /* Current size (number of elements) of the map */
	spinlock_t lock;     /* held for map lookup etc */
	ktf_map_elem_comparefn elem_comparefn; /* Key comparison function */
	ktf_map_elem_freefn elem_freefn; /* Free function */
};

struct ktf_map_elem {
	struct rb_node node;	      /* Linkage for the map */
	char key[KTF_MAX_KEY+1] __aligned(8);
		/* Key of the element - must be unique within the same map */
	struct ktf_map *map;  /* owning map */
	struct kref refcount; /* reference count for element */
};

#define __KTF_MAP_INITIALIZER(_mapname, _elem_comparefn, _elem_freefn) \
        { \
		.root = RB_ROOT, \
		.size = 0, \
		.lock = __SPIN_LOCK_UNLOCKED(_mapname), \
		.elem_comparefn = _elem_comparefn, \
		.elem_freefn = _elem_freefn, \
	}

#define DEFINE_KTF_MAP(_mapname, _elem_comparefn, _elem_freefn) \
	struct ktf_map _mapname = __KTF_MAP_INITIALIZER(_mapname, _elem_comparefn, _elem_freefn)

void ktf_map_init(struct ktf_map *map, ktf_map_elem_comparefn elem_comparefn,
	ktf_map_elem_freefn elem_freefn);

/* returns 0 upon success or -errno upon error */
int ktf_map_elem_init(struct ktf_map_elem *elem, const char *key);

/* increase/reduce reference count to element.  If count reaches 0, the
 * free function associated with map (if any) is called.
 */
void ktf_map_elem_get(struct ktf_map_elem *elem);
void ktf_map_elem_put(struct ktf_map_elem *elem);

char *ktf_map_elem_name(struct ktf_map_elem *elem, char *name);

/* Insert a new element in map - return 0 iff 'elem' was inserted or -1 if
 * the key already existed - duplicates are not insterted.
 */
int ktf_map_insert(struct ktf_map *map, struct ktf_map_elem *elem);

/* Find and return the element with 'key' */
struct ktf_map_elem *ktf_map_find(struct ktf_map *map, const char *key);

/* Find the first map elem in 'map' with reference count increased. */
struct ktf_map_elem *ktf_map_find_first(struct ktf_map *map);

/* Find the next element in the map after 'elem' if any.  Decreases refcount
 * for "elem" and increases it for returned map element - this helps manage
 * reference counts when iterating over map elements.
 */
struct ktf_map_elem *ktf_map_find_next(struct ktf_map_elem *elem);

/* Remove the element 'key' from the map and return a pointer to it with
 * refcount increased.
 */
struct ktf_map_elem *ktf_map_remove(struct ktf_map *map, const char *key);

/* Remove specific element elem from the map. Refcount is not increased
 * as caller must already have had a reference.
 */
void ktf_map_remove_elem(struct ktf_map *map, struct ktf_map_elem *elem);

void ktf_map_delete_all(struct ktf_map *map);

static inline size_t ktf_map_size(struct ktf_map *map) {
	return map->size;
}

static inline bool ktf_map_empty(struct ktf_map *map) {
	return map->size == 0;
}

/* Gets first entry with refcount of entry increased for caller. */
#define ktf_map_first_entry(_map, _type, _member) \
	ktf_map_empty(_map) ? NULL : \
	container_of(ktf_map_find_first(_map), _type, _member)

/* Gets next elem after "pos", decreasing refcount for pos and increasing
 * it for returned entry.
 */
#define ktf_map_next_entry(_pos, _member) ({ \
	struct ktf_map_elem *_e = ktf_map_find_next(&(_pos)->_member); \
        _e ? container_of(_e, typeof(*_pos), _member) : NULL; \
})

/* Iterates over map elements, incrementing refcount for current element and
 * decreasing it when we iterate to the next element.  Important - if you
 * break out of the loop via break/return, ensure ktf_map_elem_put(pos)
 * is called for current element since we have a reference to it for the
 * current loop body iteration.
 */
#define ktf_map_for_each(pos, map)	\
	for (pos = ktf_map_find_first(map); pos != NULL; pos = ktf_map_find_next(pos))

/* Iterate over map elements in similar manner as above but using
 * container_of() wrappers to work with the type embedding a
 * "struct ktf_map_elem".
 */
#define ktf_map_for_each_entry(_pos, _map, _member) \
	for (_pos = ktf_map_first_entry(_map, typeof(*_pos), _member);	\
	     _pos != NULL; \
	     _pos = ktf_map_next_entry(_pos, _member))

#define ktf_map_find_entry(_map, _key, _type, _member) ({	\
	struct ktf_map_elem *_entry = ktf_map_find(_map, _key);	\
        _entry ? container_of(_entry, _type, _member) : NULL; \
})

#endif
