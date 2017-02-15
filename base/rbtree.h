/*
  Red Black Trees
  (C) 1999  Andrea Arcangeli <andrea@suse.de>
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  linux/include/linux/rbtree.h

  To use rbtrees you'll have to implement your own insert and search cores.
  This will avoid us to use callbacks and to drop drammatically performances.
  I know it's not the cleaner way,  but in C (not in C++) to get
  performances and genericity...

  Some example of insert and search follows here. The search is a plain
  normal search over an ordered tree. The insert instead must be implemented
  int two steps: as first thing the code must insert the element in
  order as a red leaf in the tree, then the support library function
  rb_insert_color() must be called. Such function will do the
  not trivial work to rebalance the rbtree if necessary.

-----------------------------------------------------------------------
static inline struct page * rb_search_page_cache(struct inode * inode,
						 unsigned long offset)
{
	struct rb_node * n = inode->i_rb_page_cache.rb_node;
	struct page * page;

	while (n)
	{
		page = rb_entry(n, struct page, rb_page_cache);

		if (offset < page->offset)
			n = n->rb_left;
		else if (offset > page->offset)
			n = n->rb_right;
		else
			return page;
	}
	return NULL;
}

static inline struct page * __rb_insert_page_cache(struct inode * inode,
						   unsigned long offset,
						   struct rb_node * node)
{
	struct rb_node ** p = &inode->i_rb_page_cache.rb_node;
	struct rb_node * parent = NULL;
	struct page * page;

	while (*p)
	{
		parent = *p;
		page = rb_entry(parent, struct page, rb_page_cache);

		if (offset < page->offset)
			p = &(*p)->rb_left;
		else if (offset > page->offset)
			p = &(*p)->rb_right;
		else
			return page;
	}

	rb_link_node(node, parent, p);

	return NULL;
}

static inline struct page * rb_insert_page_cache(struct inode * inode,
						 unsigned long offset,
						 struct rb_node * node)
{
	struct page * ret;
	if ((ret = __rb_insert_page_cache(inode, offset, node)))
		goto out;
	rb_insert_color(node, &inode->i_rb_page_cache);
 out:
	return ret;
}
-----------------------------------------------------------------------
*/

#ifndef	_LINUX_RBTREE_H
#define	_LINUX_RBTREE_H
#ifdef _WIN32
#ifdef __cplusplus
extern "C" {
#else //__cplusplus
#define inline  __inline
#endif //__cplusplus
#endif //_WIN32

//#include <linux/kernel.h>
//#include <linux/stddef.h>

struct rb_node
{
	unsigned long  rb_parent_color;
#define	RB_RED		0
#define	RB_BLACK	1
	struct rb_node *rb_right;
	struct rb_node *rb_left;
} __attribute__((aligned(sizeof(long))));
    /* The alignment might seem pointless, but allegedly CRIS needs it */

struct rb_root
{
	struct rb_node *rb_node;
};


#define rb_parent(r)   ((struct rb_node *)((r)->rb_parent_color & ~3))
#define rb_color(r)   ((r)->rb_parent_color & 1)
#define rb_is_red(r)   (!rb_color(r))
#define rb_is_black(r) rb_color(r)

static inline void rb_set_red(struct rb_node *r)
{
	(r)->rb_parent_color &= ~1;
}
static inline void rb_set_black(struct rb_node *r)
{
	(r)->rb_parent_color |= 1;
}
static inline void rb_set_parent(struct rb_node *rb, struct rb_node *p)
{
	rb->rb_parent_color = (rb->rb_parent_color & 3) | (unsigned long)p;
}
static inline void rb_set_color(struct rb_node *rb, int color)
{
	rb->rb_parent_color = (rb->rb_parent_color & ~1) | color;
}

#define RB_ROOT	(struct rb_root) { NULL, }
#define INIT_RB_ROOT(root)       ((root)->rb_node = NULL)
#define	rb_entry(ptr, type, member) container_of(ptr, type, member)

#define RB_EMPTY_ROOT(root)	((root)->rb_node == NULL)
#define RB_EMPTY_NODE(node)	(rb_parent(node) == node)
#define RB_CLEAR_NODE(node)	(rb_set_parent(node, node))

extern void rb_insert_color(struct rb_node *, struct rb_root *);
extern void rb_erase(struct rb_node *, struct rb_root *);

/* Find logical next and previous nodes in a tree */
extern struct rb_node *rb_next(const struct rb_node *);
extern struct rb_node *rb_prev(const struct rb_node *);
extern struct rb_node *rb_first(const struct rb_root *);
extern struct rb_node *rb_last(const struct rb_root *);

/* Fast replacement of a single node without remove/rebalance/add/rebalance */
extern void rb_replace_node(struct rb_node *victim, struct rb_node *new_node, 
			    struct rb_root *root);

static inline void rb_link_node(struct rb_node * node, struct rb_node * parent,
				struct rb_node ** rb_link)
{
	node->rb_parent_color = (unsigned long )parent;
	node->rb_left = node->rb_right = NULL;

	*rb_link = node;
}

#define rb_tree_declare(type, keytype) \
	type* rb_find_##type(struct rb_root *root, keytype key); \
	type* rb_insert_##type(struct rb_root *root, type *data, keytype key); \
	type* rb_upper_##type(struct rb_root *root, keytype key); \
	type* rb_lower_##type(struct rb_root *root, keytype key);

#define rb_tree_define(type, member, keytype, keycmp) \
type* rb_find_##type(struct rb_root *root, keytype key) \
{ \
	struct rb_node *node = root->rb_node; \
	\
	while (node != NULL) \
	{ \
		type *data = rb_entry(node, type, member); \
		int result = keycmp(key, data); \
		\
		if (result < 0) \
			node = node->rb_left; \
		else if (result > 0) \
			node = node->rb_right; \
		else \
			return data; \
	} \
	return NULL; \
} \
type* rb_insert_##type(struct rb_root *root, type *data, keytype key) \
{ \
	struct rb_node **link = &(root->rb_node); \
	struct rb_node *parent = NULL; \
	\
	/* Figure out where to put new node */ \
	while (*link) \
	{ \
		type *data2 = container_of(*link, type, member); \
		int result = keycmp(key, data2); \
		\
		parent = *link; \
		if (result < 0) \
			link = &(parent->rb_left); \
		else if (result > 0) \
			link = &(parent->rb_right); \
		else \
			return data2; \
	} \
	\
	/* Add new node and rebalance tree. */ \
	rb_link_node(&data->member, parent, link); \
	rb_insert_color(&data->member, root); \
	return NULL; \
} \
type* rb_upper_##type(struct rb_root *root, keytype key) \
{ \
	struct rb_node *node = root->rb_node; \
	type *it = NULL; \
	\
	while (node != NULL) \
	{ \
		type *data = rb_entry(node, type, member); \
		int result = keycmp(key, data); \
		\
		if (result < 0) { \
			it = data; \
			node = node->rb_left; \
		} else { \
			node = node->rb_right; \
		} \
	} \
	return it; \
} \
type* rb_lower_##type(struct rb_root *root, keytype key) \
{ \
	struct rb_node *node = root->rb_node; \
	type *it = NULL; \
	\
	while (node != NULL) \
	{ \
		type *data = rb_entry(node, type, member); \
		int result = keycmp(key, data); \
		\
		if (result <= 0) { \
			it = data; \
			node = node->rb_left; \
		} else { \
			node = node->rb_right; \
		} \
	} \
	return it; \
} \

#ifdef _WIN32
#ifdef __cplusplus
};
#endif
#endif
#endif	/* _LINUX_RBTREE_H */
