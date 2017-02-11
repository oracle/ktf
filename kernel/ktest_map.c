/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Knut Omang <knut.omang@oracle.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * ktest_map.c: Implementation of a kernel version independent std::map like API
 *   (made abstract to allow impl to change)
 */

#include "ktest_map.h"
#include "ktest.h"

void ktest_map_init(struct ktest_map *map)
{
	tlog(T_INFO, "\n");
	map->root = RB_ROOT;
}

/* returns 0 upon success or -ENOMEM if key got truncated */
int ktest_elem_init(struct ktest_elem *elem, const char *name)
{
	char *dest = strncpy(elem->name, name, KTEST_MAX_NAME + 1);
	if (dest - elem->name == KTEST_MAX_NAME + 1) {
		*(--dest) = '\0';
		return -ENOMEM;
	}
	tlog(T_INFO, "\n");
	return 0;
}


struct ktest_elem *ktest_find(struct ktest_map *map, char *name)
{
	struct rb_node *node = map->root.rb_node;

	while (node) {
		struct ktest_elem *elem = container_of(node, struct ktest_elem, node);
		int result;

		result = strcmp(name, elem->name);

		if (result < 0)
			node = node->rb_left;
		else if (result > 0)
			node = node->rb_right;
		else
			return elem;
	}
        return NULL;
}

int ktest_map_insert(struct ktest_map *map, struct ktest_elem *elem)
{
	struct rb_node **newobj = &(map->root.rb_node), *parent = NULL;

	while (*newobj) {
		struct ktest_elem *this = container_of(*newobj, struct ktest_elem, node);
		int result = strcmp(elem->name, this->name);

		parent = *newobj;
		if (result < 0)
			newobj = &((*newobj)->rb_left);
		else if 	(result > 0)
			newobj = &((*newobj)->rb_right);
		else
			return -1;
	}

	/* Add newobj node and rebalance tree. */
	rb_link_node(&elem->node, parent, newobj);
	rb_insert_color(&elem->node, &map->root);

	return 0;
}

struct ktest_elem *ktest_remove(struct ktest_map *map, char *name)
{
	struct ktest_elem *elem = ktest_find(map, name);
	if (elem) {
		rb_erase(&elem->node, &map->root);
		return elem;
	}
	return NULL;
}
