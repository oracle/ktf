// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *    Author: Alan Maguire <alan.maguire@oracle.com>
 *
 * ktf_cov.h: Code coverage support interface for KTF.
 */

#ifndef KTF_COV_H
#define KTF_COV_H

#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include "ktf.h"
#include "ktf_map.h"

enum ktf_cov_type {
	KTF_COV_TYPE_MODULE,
	KTF_COV_TYPE_MAX,
};

struct ktf_cov {
	struct ktf_map_elem kmap;
	enum ktf_cov_type type;		/* only modules supported for now. */
	int count;			/* number of unique functions called */
	int total;			/* total number of functions */
	unsigned int opts;
};

/* Key for coverage entries (functions) consists in function address _and_
 * size - this allows us to find offsets in function on stack.  Also used
 * to track allocated memory - allocated address + size.
 */
struct ktf_cov_obj_key {
	unsigned long address;
	unsigned long size;
};

#define	KTF_COV_ENTRY_MAGIC		0xc07e8a5e
struct ktf_cov_entry {
	struct kprobe kprobe;
	int magic;			/* magic number identifying entry */
	struct ktf_map_elem kmap;
	char name[KTF_MAX_KEY];
	struct ktf_cov_obj_key key;
	struct ktf_cov *cov;
	struct ktf_map cov_mem;
	int refcnt;
	int count;
};

#define KTF_COV_MAX_STACK_DEPTH		32

struct ktf_cov_mem {
	struct ktf_map_elem kmap;
	struct ktf_cov_obj_key key;
	unsigned long flags;
	unsigned int nr_entries;
	unsigned long stack_entries[KTF_COV_MAX_STACK_DEPTH];
};

#define	KTF_COV_MEM_IGNORE	0x1	/* avoid recursive enter */

struct ktf_cov_mem_probe {
	const char *name;
	struct kretprobe kretprobe;
};

extern struct ktf_map cov_mem_map;

#define	ktf_for_each_cov_mem(pos)	\
	ktf_map_for_each_entry(pos, &cov_mem_map, kmap)

struct ktf_cov_entry *ktf_cov_entry_find(unsigned long, unsigned long);
void ktf_cov_entry_put(struct ktf_cov_entry *);
void ktf_cov_entry_get(struct ktf_cov_entry *);

struct ktf_cov *ktf_cov_find(const char *);
void ktf_cov_put(struct ktf_cov *);
void ktf_cov_get(struct ktf_cov *);

struct ktf_cov_mem *ktf_cov_mem_find(unsigned long, unsigned long);
void ktf_cov_mem_put(struct ktf_cov_mem *);
void ktf_cov_mem_get(struct ktf_cov_mem *);

void ktf_cov_seq_print(struct seq_file *);
void ktf_cov_cleanup(void);

int ktf_cov_enable(const char *, unsigned int);
void ktf_cov_disable(const char *);

#endif
