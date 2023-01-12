#ifndef H_BPLUS_NODE_INCLUDED
# define H_BPLUS_NODE_INCLUDED

# include <bplus.h>

# include <stdlib.h>
# include <string.h>

static inline size_t node_type_size (size_t *alignment);

  /* keys must have order - 1 allocated, values must have order
     allocated. The pointer must remain valid, because they will be
     used as the "keys" and "values" of the node. */
static inline void node_setup (struct bplus_node *node,
			       size_t order,
			       uint32_t * restrict keys,
			       uint32_t * restrict values);

  /* The nodes must have the same order, obviously. */
static inline void node_copy (size_t order,
			      struct bplus_node *dest,
			      const struct bplus_node *src);

static inline int node_is_leaf (const struct bplus_node *node);
static inline void node_set_leaf (struct bplus_node *node);
static inline void node_set_non_leaf (struct bplus_node *node);

  /* Return a non-zero value if this is the root. Otherwise, set
   *parent to the parent. */
static inline int node_get_parent (const struct bplus_node
				   *node, uint32_t * parent);
static inline void node_set_parent (struct bplus_node *node, uint32_t parent);
static inline void node_unset_parent (struct bplus_node *node);

static inline size_t node_count_entries (const struct bplus_node *node);

  /* For inner nodes, next_value is set to the first child going
     strictly after the key for entry, and for leaves, it is set to a
     meaningless value. You can pass NULL in either cases. Bounds are
     not checked. */
static inline void node_get_entry (const struct bplus_node
				   *node, size_t entry,
				   uint32_t * key,
				   uint32_t * value, uint32_t * next_value);

static inline void node_set_entry (struct bplus_node *node,
				   size_t entry, uint32_t key,
				   uint32_t value);

static inline void node_truncate (struct bplus_node *node, size_t n_entries);
# include <string.h>

static inline size_t
node_type_size (size_t *alignment)
{
  if (alignment)
    {
# ifdef ALIGNOF_STRUCT_BPLUS_NODE
      *alignment = ALIGNOF_STRUCT_BPLUS_NODE;
# else/* not ALIGNOF_STRUCT_BPLUS_KEY */
      *alignment = 16;
# endif/* not ALIGNOF_STRUCT_BPLUS_KEY */
    }
  return sizeof (struct bplus_node);
}

static inline void
node_setup (struct bplus_node *node, size_t order,
	    uint32_t * restrict keys, uint32_t * restrict values)
{
  (void) order;
  node->n_entries = 0;
  node->keys = keys;
  node->values = values;
  node->is_leaf = 0;
  node->parent_node = ((uint32_t) (-1));
}

static inline void
node_copy (size_t order, struct bplus_node *dest,
	   const struct bplus_node *src)
{
  dest->n_entries = src->n_entries;
  memcpy (dest->keys, src->keys, src->n_entries * sizeof (uint32_t));
  memcpy (dest->values, src->values, src->n_entries * sizeof (uint32_t));
  if (src->is_leaf)
    {
      dest->values[order - 1] = src->values[order - 1];
    }
  else
    {
      dest->values[src->n_entries] = src->values[src->n_entries];
    }
  dest->is_leaf = src->is_leaf;
  dest->parent_node = src->parent_node;
}

static inline int
node_is_leaf (const struct bplus_node *node)
{
  return node->is_leaf;
}

static inline void
node_set_leaf (struct bplus_node *node)
{
  node->is_leaf = 1;
}

static inline void
node_set_non_leaf (struct bplus_node *node)
{
  node->is_leaf = 0;
}

static inline int
node_get_parent (const struct bplus_node *node, uint32_t * parent)
{
  if (parent)
    {
      *parent = node->parent_node;
    }
  if (node->parent_node == ((uint32_t) (-1)))
    {
      return 1;
    }
  return 0;
}

static inline void
node_set_parent (struct bplus_node *node, uint32_t parent)
{
  node->parent_node = parent;
}

static inline void
node_unset_parent (struct bplus_node *node)
{
  node->parent_node = ((uint32_t) (-1));
}

static inline size_t
node_count_entries (const struct bplus_node *node)
{
  return node->n_entries;
}

static inline void
node_get_entry (const struct bplus_node *node, size_t entry,
		uint32_t * key, uint32_t * value, uint32_t * next_value)
{
  if (key)
    {
      *key = node->keys[entry];
    }
  if (value)
    {
      *value = node->values[entry];
    }
  if (next_value)
    {
      if (node->is_leaf)
	{
	  *next_value = 0;
	}
      else
	{
	  *next_value = node->values[entry + 1];
	}
    }
}

static inline void
node_set_entry (struct bplus_node *node, size_t entry, uint32_t key,
		uint32_t value)
{
  node->keys[entry] = key;
  node->values[entry] = value;
}

static inline void
node_truncate (struct bplus_node *node, size_t n_entries)
{
  node->n_entries = n_entries;
}

#endif /* not H_BPLUS_NODE_INCLUDED */
