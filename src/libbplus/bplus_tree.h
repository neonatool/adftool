#ifndef H_BPLUS_TREE_INCLUDED
# define H_BPLUS_TREE_INCLUDED

# include <bplus.h>

# include <stdlib.h>
# include <string.h>

# define DEALLOC_TREE \
  ATTRIBUTE_DEALLOC (tree_free, 1)

struct bplus_tree;

static void tree_free (struct bplus_tree *tree);

DEALLOC_TREE static struct bplus_tree *tree_alloc (size_t order);

static inline size_t tree_order (struct bplus_tree *tree);

static inline void tree_cache (struct bplus_tree *tree,
			       uint32_t id, const struct bplus_node *node);

static inline
  const struct bplus_node *tree_get_cache (const struct bplus_tree *tree,
					   uint32_t id);

# include "bplus_node.h"

# ifndef DEFAULT_TREE_CACHE
#  define DEFAULT_TREE_CACHE 251
# endif/* not DEFAULT_TREE_CACHE */

struct bplus_tree
{
  size_t order;
  struct bplus_node *node_cache;
  size_t cache_size;
  uint32_t *node_ids;		/* cache_size elements allocated, -1 means the
				   cache item is free */
  uint32_t *node_keys;		/* (order - 1) * cache_size elements allocated */
  uint32_t *node_values;
};

static struct bplus_tree *
tree_alloc (size_t order)
{
  struct bplus_tree *tree = malloc (sizeof (struct bplus_tree));
  if (tree != NULL)
    {
      tree->order = order;
      tree->cache_size = DEFAULT_TREE_CACHE;
      tree->node_cache =
	malloc (tree->cache_size * sizeof (struct bplus_node));
      tree->node_ids = malloc (tree->cache_size * sizeof (uint32_t));
      tree->node_keys =
	malloc ((order - 1) * tree->cache_size * sizeof (uint32_t));
      tree->node_values =
	malloc (order * tree->cache_size * sizeof (uint32_t));
      if (tree->node_cache == NULL || tree->node_ids == NULL
	  || tree->node_keys == NULL || tree->node_values == NULL)
	{
	  free (tree->node_cache);
	  free (tree->node_ids);
	  free (tree->node_keys);
	  free (tree->node_values);
	  free (tree);
	  tree = NULL;
	}
    }
  if (tree != NULL)
    {
      for (size_t i = 0; i < tree->cache_size; i++)
	{
	  tree->node_cache[i].n_entries = 0;
	  tree->node_cache[i].keys = tree->node_keys + i * (order - 1);
	  tree->node_cache[i].values = tree->node_values + i * order;
	  tree->node_ids[i] = (uint32_t) (-1);
	}
    }
  return tree;
}

static void
tree_free (struct bplus_tree *tree)
{
  if (tree != NULL)
    {
      free (tree->node_cache);
      free (tree->node_ids);
      free (tree->node_keys);
      free (tree->node_values);
    }
  free (tree);
}

static inline size_t
tree_order (struct bplus_tree *tree)
{
  return tree->order;
}

static inline size_t
id_hash (size_t cache_size, uint32_t id)
{
  return id % cache_size;
}

static inline void
tree_cache (struct bplus_tree *tree, uint32_t id,
	    const struct bplus_node *node)
{
  const size_t position = id_hash (tree->cache_size, id);
  if (node == NULL)
    {
      id = ((uint32_t) (-1));
    }
  else
    {
      node_copy (tree->order, &(tree->node_cache[position]), node);
    }
  tree->node_ids[position] = id;
}

static inline const struct bplus_node *
tree_get_cache (const struct bplus_tree *tree, uint32_t id)
{
  const size_t position = id_hash (tree->cache_size, id);
  if (tree->node_ids[position] == id)
    {
      return &(tree->node_cache[position]);
    }
  return NULL;
}

#endif /* not H_BPLUS_TREE_INCLUDED */
