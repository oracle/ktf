// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * ktf_map.c: Implementation of a kernel version independent std::map like API
 *   (made abstract to allow impl to change)
 */

#include "ktf_map.h"
#include "ktf.h"
#include "ktf_compat.h"

void ktf_map_init(struct ktf_map *map, ktf_map_elem_comparefn elem_comparefn,
		  ktf_map_elem_freefn elem_freefn)
{
	map->root = RB_ROOT;
	map->size = 0;
	map->elem_comparefn = elem_comparefn;
	map->elem_freefn = elem_freefn;
	spin_lock_init(&map->lock);
}

int ktf_map_elem_init(struct ktf_map_elem *elem, const char *key)
{
	memcpy(elem->key, key, KTF_MAX_KEY);
	/* For strings that are too long, ensure truncation at
	 * KTF_MAX_NAME == KTF_MAX_KEY - 1 length:
	 */
	elem->key[KTF_MAX_NAME] = '\0';
	elem->map = NULL;
	kref_init(&elem->refcount);
	return 0;
}

/* A convenience unsigned int compare function as an alternative
 * to the string compare:
 */
int ktf_uint_compare(const char *ac, const char *bc)
{
	unsigned int a = *((unsigned int *)ac);
	unsigned int b = *((unsigned int *)bc);

	return a > b ? 1 : (a < b ? -1 : 0);
}
EXPORT_SYMBOL(ktf_uint_compare);

/* Copy "elem"s key representation into "name".  For cases where no
 * compare function is defined - i.e. string keys - just copy string,
 * otherwise name is hexascii of first 8 bytes of key.
 */
char *
ktf_map_elem_name(struct ktf_map_elem *elem, char *name)
{
	if (!name)
		return NULL;

	if (!elem || !elem->map)
		(void)strscpy(name, "<none>", KTF_MAX_NAME);
	else if (!elem->map->elem_comparefn)
		(void)strscpy(name, elem->key, KTF_MAX_NAME);
	else
		(void)snprintf(name, KTF_MAX_NAME, "'%*ph'", 8, elem->key);

	return name;
}

/* Called when refcount of elem is 0. */
static void ktf_map_elem_release(struct kref *kref)
{
	struct ktf_map_elem *elem = container_of(kref, struct ktf_map_elem,
						 refcount);
	struct ktf_map *map = elem->map;
	char name[KTF_MAX_KEY];

	tlog(T_DEBUG_V, "Releasing %s, %s free function",
	     ktf_map_elem_name(elem, name),
	     map && map->elem_freefn ? "calling" : "no");
	if (map && map->elem_freefn)
		map->elem_freefn(elem);
}

void ktf_map_elem_put(struct ktf_map_elem *elem)
{
	char name[KTF_MAX_KEY];

	tlog(T_DEBUG_V, "Decreasing refcount for %s to %d",
	     ktf_map_elem_name(elem, name),
	     refcount_read(&elem->refcount.refcount) - 1);
	kref_put(&elem->refcount, ktf_map_elem_release);
}

void ktf_map_elem_get(struct ktf_map_elem *elem)
{
	char name[KTF_MAX_KEY];

	tlog(T_DEBUG_V, "Increasing refcount for %s to %d",
	     ktf_map_elem_name(elem, name),
	     refcount_read(&elem->refcount.refcount) + 1);
	kref_get(&elem->refcount);
}

struct ktf_map_elem *ktf_map_find(struct ktf_map *map, const char *key)
{
	struct rb_node *node;
	unsigned long flags;

	/* may be called in interrupt context */
	spin_lock_irqsave(&map->lock, flags);
	node = map->root.rb_node;

	while (node) {
		struct ktf_map_elem *elem = container_of(node, struct ktf_map_elem, node);
		int result;

		if (map->elem_comparefn)
			result = map->elem_comparefn(key, elem->key);
		else
			result = strncmp(key, elem->key, KTF_MAX_KEY);

		if (result < 0) {
			node = node->rb_left;
		} else if (result > 0) {
			node = node->rb_right;
		} else {
			ktf_map_elem_get(elem);
			spin_unlock_irqrestore(&map->lock, flags);
			return elem;
		}
	}
	spin_unlock_irqrestore(&map->lock, flags);
	return NULL;
}

/* Find the first map elem in 'map' */
struct ktf_map_elem *ktf_map_find_first(struct ktf_map *map)
{
	struct ktf_map_elem *elem = NULL;
	struct rb_node *node;
	unsigned long flags;

	spin_lock_irqsave(&map->lock, flags);
	node = rb_first(&map->root);
	if (node) {
		elem = container_of(node, struct ktf_map_elem, node);
		ktf_map_elem_get(elem);
	}
	spin_unlock_irqrestore(&map->lock, flags);
	return elem;
}

/* Find the next element in the map after 'elem' if any */
struct ktf_map_elem *ktf_map_find_next(struct ktf_map_elem *elem)
{
	struct ktf_map_elem *next = NULL;
	struct ktf_map *map = elem->map;
	struct rb_node *node;
	unsigned long flags;

	if (!elem->map)
		return NULL;
	spin_lock_irqsave(&map->lock, flags);
	node = rb_next(&elem->node);

	/* Assumption here - we don't need ref to elem any more.
	 * Common usage pattern is
	 *
	 * for (elem = ktf_map_elem_first(map); elem != NULL;
	 *      elem = ktf_map_find_next(elem))
	 *
	 * but if other use cases occur we may need to revisit.
	 * This assumption allows us to define our _for_each macros
	 * and still manage refcounts.
	 */
	ktf_map_elem_put(elem);

	if (node) {
		next = container_of(node, struct ktf_map_elem, node);
		ktf_map_elem_get(next);
	}
	spin_unlock_irqrestore(&map->lock, flags);
	return next;
}

int ktf_map_insert(struct ktf_map *map, struct ktf_map_elem *elem)
{
	struct rb_node **newobj, *parent = NULL;
	unsigned long flags;

	spin_lock_irqsave(&map->lock, flags);
	newobj = &map->root.rb_node;
	while (*newobj) {
		struct ktf_map_elem *this = container_of(*newobj, struct ktf_map_elem, node);
		int result;

		if (map->elem_comparefn)
			result = map->elem_comparefn(elem->key, this->key);
		else
			result = strncmp(elem->key, this->key, KTF_MAX_KEY);

		parent = *newobj;
		if (result < 0) {
			newobj = &((*newobj)->rb_left);
		} else if (result > 0) {
			newobj = &((*newobj)->rb_right);
		} else {
			spin_unlock_irqrestore(&map->lock, flags);
			return -EEXIST;
		}
	}

	/* Add newobj node and rebalance tree. */
	rb_link_node(&elem->node, parent, newobj);
	rb_insert_color(&elem->node, &map->root);
	elem->map = map;
	map->size++;
	/* Bump reference count for map reference */
	ktf_map_elem_get(elem);
	spin_unlock_irqrestore(&map->lock, flags);
	return 0;
}

void ktf_map_remove_elem(struct ktf_map *map, struct ktf_map_elem *elem)
{
	if (elem) {
		rb_erase(&elem->node, &map->root);
		map->size--;
		ktf_map_elem_put(elem);
	}
}

struct ktf_map_elem *ktf_map_remove(struct ktf_map *map, const char *key)
{
	struct ktf_map_elem *elem;
	unsigned long flags;

	elem = ktf_map_find(map, key);
	spin_lock_irqsave(&map->lock, flags);
	ktf_map_remove_elem(map, elem);
	spin_unlock_irqrestore(&map->lock, flags);
	return elem;
}

void ktf_map_delete_all(struct ktf_map *map)
{
	struct ktf_map_elem *elem;
	struct rb_node *node;
	unsigned long flags;

	spin_lock_irqsave(&map->lock, flags);
	do {
		node = rb_first(&(map)->root);
		if (node) {
			rb_erase(node, &(map)->root);
			map->size--;
			elem = container_of(node, struct ktf_map_elem, node);
			ktf_map_elem_put(elem);
		}
	} while (node);
	spin_unlock_irqrestore(&map->lock, flags);
}
