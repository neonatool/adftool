#ifndef H_ADFTOOL_BPLUS_NODE_INCLUDED
#define H_ADFTOOL_BPLUS_NODE_INCLUDED

#include <adftool_private.h>

struct node
{
  uint32_t id;
  size_t order;
  uint32_t *row;		/* 2 * order + 1 elements allocated. */
};

static inline int node_init (size_t order, uint32_t id, struct node *node);
static inline void node_clean (struct node *node);
static inline int node_set_order (struct node *node, size_t order);
static inline int node_is_leaf (const struct node *node);
static inline void node_set_leaf (struct node *node);
static inline void node_set_non_leaf (struct node *node);
static inline uint32_t node_key (const struct node *node, size_t i);
static inline void node_set_key (struct node *node, size_t i, uint32_t key);
static inline uint32_t node_value (const struct node *node, size_t i);
static inline void node_set_value (struct node *node, size_t i,
				   uint32_t value);
static inline uint32_t node_next_leaf (const struct node *node);
static inline void node_set_next_leaf (struct node *node, uint32_t next_leaf);
static inline uint32_t node_parent (const struct node *node);
static inline void node_set_parent (struct node *node, uint32_t parent);
static inline int node_fetch (struct bplus *bplus, uint32_t id,
			      struct node *node);
static inline void node_store (struct bplus *bplus, const struct node *node);

static inline int
node_init (size_t order, uint32_t id, struct node *node)
{
  node->id = id;
  node->order = order;
  node->row = malloc ((2 * node->order + 1) * sizeof (uint32_t));
  if (node->row == NULL)
    {
      return 1;
    }
  assert (order > 0);
  for (size_t i = 0; i + 1 < order; i++)
    {
      node->row[i] = (uint32_t) (-1);
    }
  for (size_t i = order - 1; i < 2 * order - 1; i++)
    {
      node->row[i] = 0;
    }
  node->row[2 * order - 1] = -1;
  node->row[2 * order] = 0;
  return 0;
}

static inline void
node_clean (struct node *node)
{
  free (node->row);
}

static inline int
node_set_order (struct node *node, size_t order)
{
  if (order != node->order)
    {
      /* Realloc the row */
      uint32_t *reallocated =
	realloc (node->row, (2 * order + 1) * sizeof (uint32_t));
      if (reallocated == NULL)
	{
	  return 1;
	}
      assert (order > 0);
      for (size_t i = 0; i + 1 < order; i++)
	{
	  reallocated[i] = (uint32_t) (-1);
	}
      for (size_t i = order - 1; i < 2 * order - 1; i++)
	{
	  reallocated[i] = 0;
	}
      reallocated[2 * order - 1] = -1;
      reallocated[2 * order] = 0;
      node->order = order;
      node->row = reallocated;
    }
  return 0;
}

static inline int
node_is_leaf (const struct node *node)
{
  uint32_t flags = node->row[2 * node->order];
  uint32_t bit = 1;
  uint32_t flag_is_leaf = bit << 31;
  return (flags & flag_is_leaf) != 0;
}

static inline void
node_set_leaf (struct node *node)
{
  uint32_t bit = 1;
  uint32_t flag_is_leaf = bit << 31;
  node->row[2 * node->order] = flag_is_leaf;
}

static inline void
node_set_non_leaf (struct node *node)
{
  node->row[2 * node->order] = 0;
}

static inline uint32_t
node_key (const struct node *node, size_t i)
{
  assert (i + 1 < node->order);
  return node->row[i];
}

static inline void
node_set_key (struct node *node, size_t i, uint32_t key)
{
  assert (i + 1 < node->order);
  node->row[i] = key;
}

static inline uint32_t
node_value (const struct node *node, size_t i)
{
  assert (i < node->order);
  return node->row[node->order - 1 + i];
}

static inline void
node_set_value (struct node *node, size_t i, uint32_t value)
{
  assert (i < node->order);
  node->row[node->order - 1 + i] = value;
}

static inline uint32_t
node_next_leaf (const struct node *node)
{
  assert (node_is_leaf (node));
  assert (node->order > 0);
  return node_value (node, node->order - 1);
}

static inline void
node_set_next_leaf (struct node *node, uint32_t next_leaf)
{
  assert (node_is_leaf (node));
  node_set_value (node, node->order - 1, next_leaf);
}

static inline uint32_t
node_parent (const struct node *node)
{
  assert (node->order > 0);
  return node->row[2 * node->order - 1];
}

static inline void
node_set_parent (struct node *node, uint32_t parent)
{
  assert (node->order > 0);
  node->row[2 * node->order - 1] = parent;
}

static inline int
node_fetch (struct bplus *bplus, uint32_t id, struct node *node)
{
  size_t actual_row_length;
  node->id = id;
  while (1)
    {
      int fetch_error = bplus_fetch (bplus, node->id, &actual_row_length, 0,
				     2 * node->order + 1, node->row);
      if (fetch_error)
	{
	  return 1;
	}
      else if (actual_row_length != 2 * node->order + 1)
	{
	  assert ((actual_row_length % 2) == 1);
	  int grow_error = node_set_order (node, (actual_row_length - 1) / 2);
	  if (grow_error)
	    {
	      return 1;
	    }
	  fetch_error =
	    bplus_fetch (bplus, node->id, &actual_row_length, 0,
			 2 * node->order + 1, node->row);
	  if (fetch_error)
	    {
	      return 1;
	    }
	  else if (actual_row_length == 2 * node->order + 1)
	    {
	      break;
	    }
	}
      else
	{
	  break;
	}
    }
  return 0;
}

static inline void
node_store (struct bplus *bplus, const struct node *node)
{
  bplus_store (bplus, node->id, 0, 2 * node->order + 1, node->row);
}

#endif /* not H_ADFTOOL_BPLUS_NODE_INCLUDED */
