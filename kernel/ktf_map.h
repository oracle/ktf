/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * ktf_map.h: simple objects with name lookup to be inlined into a
 *    larger object
 */

#ifndef _KTF_MAP_H
#define _KTF_MAP_H
#include <linux/version.h>
#include <linux/rbtree.h>

#define KTF_MAX_NAME 64

struct ktf_map {
	struct rb_root root; /* The rb tree holding the map */
	size_t size;	     /* Current size (number of elements) of the map */
};

struct ktf_map_elem {
	struct rb_node node;	      /* Linkage for the map */
	char name[KTF_MAX_NAME+1];  /* Name of the element - must be unique within the same map */
};


#define DEFINE_KTF_MAP(_mapname) \
	struct ktf_map _mapname = { \
		.root = RB_ROOT, \
		.size = 0, \
	}

void ktf_map_init(struct ktf_map *map);

/* returns 0 upon success or -errno upon error */
int ktf_map_elem_init(struct ktf_map_elem *elem, const char *name);

/* Insert a new element in map - return 0 iff 'elem' was inserted or -1 if
 * the key already existed - duplicates are not insterted.
 */
int ktf_map_insert(struct ktf_map *map, struct ktf_map_elem *elem);

/* Find and return the element 'name' */
struct ktf_map_elem *ktf_map_find(struct ktf_map *map, const char *name);

/* Find the first map elem in 'map' */
struct ktf_map_elem *ktf_map_find_first(struct ktf_map *map);

/* Find the next element in the map after 'elem' if any */
struct ktf_map_elem *ktf_map_find_next(struct ktf_map_elem *elem);

/* Remove the element 'name' from the map and return a pointer to it */
struct ktf_map_elem *ktf_map_remove(struct ktf_map *map, const char *name);

static inline size_t ktf_map_size(struct ktf_map *map) {
	return map->size;
}

static inline bool ktf_map_empty(struct ktf_map *map) {
	return map->size == 0;
}

#define ktf_map_first_entry(map, type, member) \
	ktf_map_empty(map) ? NULL : \
	container_of(ktf_map_find_first(map), type, member)

#define ktf_map_next_entry(pos, member) ({ \
	struct ktf_map_elem *e = ktf_map_find_next(&(pos)->member); \
        e ? container_of(e, typeof(*pos), member) : NULL; \
})

#define ktf_map_for_each(pos, map)	\
	for (pos = ktf_map_find_first(map); pos != NULL; pos = ktf_map_find_next(pos))

#define ktf_map_for_each_entry(pos, map, member) \
	for (pos = ktf_map_first_entry(map, typeof(*pos), member);	\
	     pos != NULL; \
	     pos = ktf_map_next_entry(pos, member))

#define ktf_map_find_entry(map, key, type, member) ({	\
	struct ktf_map_elem *e = ktf_map_find(map, key);	\
        e ? container_of(e, type, member) : NULL; \
})

#if (KERNEL_VERSION(3, 11, 0) > LINUX_VERSION_CODE)
/* postorder traversal not supported, just remove from the head */
#define ktf_map_delete_all(map, type, member) { \
	struct ktf_map_elem *elem; \
	struct rb_node *node; \
	type *e; \
	do { \
		node = rb_first(&(map)->root);\
		if (node) { \
			rb_erase(node, &(map)->root); \
			elem = container_of(node, struct ktf_map_elem, node);	\
			e = container_of(elem, type, member);\
			kfree(e);  \
		}\
	} while (node != NULL);	\
}
#else

/* Empty the whole map by first calling kfree on each entry in post order,
 * then setting the map to empty
 */

#define ktf_map_delete_all(map, type, member) { \
	struct ktf_map_elem *elem, *pos; \
	rbtree_postorder_for_each_entry_safe(pos, elem, &(map)->root, node) { \
		type *e = container_of(pos, type, member);\
		kfree(e); \
	}\
}

#endif /* LINUX_VERSION_CODE */
#endif
