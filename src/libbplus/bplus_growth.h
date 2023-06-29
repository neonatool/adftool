#ifndef H_BPLUS_GROWTH_INCLUDED
# define H_BPLUS_GROWTH_INCLUDED

# include <bplus.h>

# include <stdlib.h>
# include <string.h>
# include <assert.h>

# include "bplus_reparentor.h"
# include "bplus_fetcher.h"

# define DEALLOC_GROWTH \
  ATTRIBUTE_DEALLOC (growth_free, 1)

struct bplus_growth;

static void growth_free (struct bplus_growth *growth);

DEALLOC_GROWTH
  static struct bplus_growth *growth_alloc (struct bplus_tree *tree);

static void growth_setup (struct bplus_growth *growth);

static void growth_status (const struct bplus_growth *growth, int *done,
			   /* When this is done, if you have a
			      local variable whose ID is 0, its
			      parent should be -1. Replace its
			      parent with 0, and change its ID to
			      new_id. */
			   uint32_t * new_id,
			   /* The growth operation will need you to
			      fetch some data. */
			   size_t *n_fetches_to_do,
			   size_t start_fetch_to_do,
			   size_t max_fetches_to_do,
			   size_t *fetch_rows,
			   size_t *fetch_starts, size_t *fetch_lengths,
			   /* It will also require you to allocate
			      new nodes. */
			   size_t *n_allocations_to_do,
			   /* You will also have to update the
			      storage. */
			   size_t *n_stores_to_do,
			   size_t start_store_to_do,
			   size_t max_stores_to_do,
			   size_t *store_rows,
			   size_t *store_starts,
			   size_t *store_lengths, const uint32_t ** stores);

static void growth_data (struct bplus_growth *growth, size_t row,
			 size_t start, size_t length, const uint32_t * data);

static void growth_updated (struct bplus_growth *growth, size_t row);

  /* Return 0 if the allocation has not been claimed by the growth, 1
     if it has. */
static int growth_allocated (struct bplus_growth *growth,
			     uint32_t new_node_id);

struct bplus_growth
{
  size_t order;
  struct bplus_tree *tree;

  int root_fetched;		/* Indicates whether the updates have been
				   started. */
  int node_0_updated;		/* Indicates whether node 0 update has been
				   taken care of. */
  int new_root_allocated;	/* Set to 1 if the new root has been
				   allocated. */
  int new_root_saved;		/* Set to 1 when the new root has been
				   allocated, and saved. */
  int new_root_children_fixed;	/* Set to 1 if the new root’s children
				   have been fixed. */

  struct bplus_fetcher *old_root_fetcher;
  const struct bplus_node *old_root;
  uint32_t *node_0_update;
  uint32_t new_node_id;
  uint32_t *new_node_update;
  struct bplus_reparentor *fix_children;
};

static struct bplus_growth *
growth_alloc (struct bplus_tree *tree)
{
  struct bplus_growth *ret = malloc (sizeof (struct bplus_growth));
  if (ret == NULL)
    {
      return NULL;
    }
  ret->order = bplus_tree_order (tree);
  ret->tree = tree;
  ret->old_root_fetcher = fetcher_alloc (tree);
  ret->node_0_update = malloc ((2 * ret->order + 1) * sizeof (uint32_t));
  ret->new_node_update = malloc ((2 * ret->order + 1) * sizeof (uint32_t));
  ret->fix_children = reparentor_alloc (tree);
  if (ret->old_root_fetcher == NULL || ret->node_0_update == NULL
      || ret->new_node_update == NULL || ret->fix_children == NULL)
    {
      fetcher_free (ret->old_root_fetcher);
      free (ret->node_0_update);
      free (ret->new_node_update);
      reparentor_free (ret->fix_children);
      free (ret);
      return NULL;
    }
  return ret;
}

static void
growth_free (struct bplus_growth *growth)
{
  if (growth != NULL)
    {
      fetcher_free (growth->old_root_fetcher);
      free (growth->node_0_update);
      free (growth->new_node_update);
      reparentor_free (growth->fix_children);
    }
  free (growth);
}

static void
growth_when_root_fetched_and_allocated (struct bplus_growth *growth)
{
  /* Called the first time that the old root has been fetched, and
     that the new node has been allocated. */
  assert (growth->root_fetched == 1);
  assert (growth->new_root_allocated == 1);
  assert (growth->node_0_updated == 0);
  assert (growth->new_root_saved == 0);
  assert (growth->new_root_children_fixed == 0);
  const size_t order = growth->order;
  /* Compute the node 0 update: */
  growth->node_0_updated = 0;
  for (size_t i = 0; i + 1 < order; i++)
    {
      /* Set all keys to -1, and all children but the first to 0. */
      growth->node_0_update[i] = ((uint32_t) (-1));
      growth->node_0_update[order - 1 + (i + 1)] = 0;
    }
  /* Set the only child: */
  growth->node_0_update[order - 1] = growth->new_node_id;
  /* No parent: */
  growth->node_0_update[2 * order - 1] = ((uint32_t) (-1));
  /* Not a leaf: */
  growth->node_0_update[2 * order] = 0;
  /* Compute the newly allocated node update: */
  growth->new_root_saved = 0;
  for (size_t i = 0; i + 1 < order; i++)
    {
      uint32_t key = ((uint32_t) (-1));
      uint32_t value = 0;
      if (i < growth->old_root->n_entries)
	{
	  key = growth->old_root->keys[i];
	  value = growth->old_root->values[i];
	}
      growth->new_node_update[i] = key;
      growth->new_node_update[order - 1 + i] = value;
    }
  growth->new_node_update[2 * order - 2] = 0;
  if (growth->old_root->is_leaf)
    {
      growth->new_node_update[2 * order - 2] =
	growth->old_root->values[order - 1];
      growth->new_node_update[2 * order] = ((uint32_t) ((uint32_t) 1) << 31);
    }
  else
    {
      growth->new_node_update[order - 1 + growth->old_root->n_entries] =
	growth->old_root->values[growth->old_root->n_entries];
      growth->new_node_update[2 * order] = 0;
    }
  /* The parent is node 0: */
  growth->new_node_update[2 * order - 1] = 0;
  /* Set up the child updater: */
  growth->new_root_children_fixed = 0;
  reparentor_setup (growth->fix_children, growth->new_node_id,
		    growth->old_root);
}

static void
growth_advance (struct bplus_growth *growth)
{
  if (!(growth->root_fetched))
    {
      const struct bplus_node *root =
	fetcher_status (growth->old_root_fetcher, NULL, NULL, NULL);
      if (root != NULL)
	{
	  growth->root_fetched = 1;
	  growth->old_root = root;
	  if (growth->new_root_allocated)
	    {
	      growth_when_root_fetched_and_allocated (growth);
	      growth_advance (growth);
	    }
	}
    }
  else if (growth->root_fetched && growth->new_root_allocated
	   && !(growth->new_root_children_fixed))
    {
      int children_fix_done;
      size_t n_fetches_to_do, n_stores_to_do;
      reparentor_status (growth->fix_children, &children_fix_done,
			 &n_fetches_to_do, 0, 0, NULL, NULL, NULL,
			 &n_stores_to_do, 0, 0, NULL, NULL, NULL, NULL);
      if (children_fix_done)
	{
	  growth->new_root_children_fixed = 1;
	}
    }
}

static int
growth_advance_with_allocation (struct bplus_growth *growth,
				uint32_t allocated)
{
  if (!(growth->new_root_allocated))
    {
      growth->new_root_allocated = 1;
      growth->new_node_id = allocated;
      if (growth->root_fetched)
	{
	  growth_when_root_fetched_and_allocated (growth);
	  growth_advance (growth);
	}
      return 1;
    }
  return 0;
}

static void
growth_setup (struct bplus_growth *growth)
{
  growth->root_fetched = 0;
  growth->node_0_updated = 0;
  growth->new_root_allocated = 0;
  growth->new_root_saved = 0;
  growth->new_root_children_fixed = 0;
  fetcher_setup (growth->old_root_fetcher, 0);
  growth_advance (growth);
}

static void
growth_data (struct bplus_growth *growth, size_t row, size_t start,
	     size_t length, const uint32_t * data)
{
  if (!(growth->root_fetched))
    {
      fetcher_data (growth->old_root_fetcher, row, start, length, data);
      growth_advance (growth);
    }
  else if (growth->root_fetched && growth->new_root_allocated
	   && !(growth->new_root_children_fixed))
    {
      reparentor_data (growth->fix_children, row, start, length, data);
      growth_advance (growth);
    }
}

static void
growth_updated (struct bplus_growth *growth, size_t row)
{
  if (growth->root_fetched && growth->new_root_allocated
      && !(growth->new_root_children_fixed))
    {
      reparentor_updated (growth->fix_children, row);
      growth_advance (growth);
    }
  if (growth->root_fetched && growth->new_root_allocated
      && !(growth->node_0_updated) && row == 0)
    {
      growth->node_0_updated = 1;
      bplus_tree_cache (growth->tree, 0, NULL);
      growth_advance (growth);
    }
  else if (growth->root_fetched && growth->new_root_allocated
	   && !(growth->new_root_saved) && row == growth->new_node_id)
    {
      growth->new_root_saved = 1;
      bplus_tree_cache (growth->tree, growth->new_node_id, NULL);
      growth_advance (growth);
    }
}

static int
growth_allocated (struct bplus_growth *growth, uint32_t new_node_id)
{
  return growth_advance_with_allocation (growth, new_node_id);
}

static void
growth_status (const struct bplus_growth *growth, int *done,
	       uint32_t * new_id, size_t *n_fetches_to_do,
	       size_t start_fetch_to_do, size_t max_fetches_to_do,
	       size_t *fetch_rows, size_t *fetch_starts,
	       size_t *fetch_lengths, size_t *n_allocations_to_do,
	       size_t *n_stores_to_do, size_t start_store_to_do,
	       size_t max_stores_to_do, size_t *store_rows,
	       size_t *store_starts, size_t *store_lengths,
	       const uint32_t ** stores)
{
  const size_t order = growth->order;
  *done = 0;
  *n_fetches_to_do = 0;
  *n_allocations_to_do = 0;
  *n_stores_to_do = 0;
  if (growth->root_fetched && growth->new_root_allocated
      && growth->node_0_updated && growth->new_root_saved
      && growth->new_root_children_fixed)
    {
      *done = 1;
      *new_id = growth->new_node_id;
    }
  else if (growth->root_fetched && growth->new_root_allocated)
    {
      /* Both updates are allocated, and the reparentor is set up. */
      if (!(growth->new_root_children_fixed))
	{
	  int reparentor_done;
	  reparentor_status (growth->fix_children, &reparentor_done,
			     n_fetches_to_do, start_fetch_to_do,
			     max_fetches_to_do, fetch_rows, fetch_starts,
			     fetch_lengths, n_stores_to_do, start_store_to_do,
			     max_stores_to_do, store_rows, store_starts,
			     store_lengths, stores);
	  assert (!reparentor_done);
	  /* Otherwise, we failed to call
	     growth_advance() before that. */
	}
      if (!(growth->node_0_updated))
	{
	  /* Push an update. */
	  size_t i = *n_stores_to_do;
	  if (i >= start_store_to_do
	      && i - start_store_to_do < max_stores_to_do)
	    {
	      const size_t index = i - start_store_to_do;
	      store_rows[index] = 0;
	      store_starts[index] = 0;
	      store_lengths[index] = 2 * order + 1;
	      stores[index] = growth->node_0_update;
	    }
	  *n_stores_to_do = i + 1;
	}
      if (!(growth->new_root_saved))
	{
	  /* Push an update. */
	  size_t i = *n_stores_to_do;
	  if (i >= start_store_to_do
	      && i - start_store_to_do < max_stores_to_do)
	    {
	      const size_t index = i - start_store_to_do;
	      store_rows[index] = growth->new_node_id;
	      store_starts[index] = 0;
	      store_lengths[index] = 2 * order + 1;
	      stores[index] = growth->new_node_update;
	    }
	  *n_stores_to_do = i + 1;
	}
    }
  else
    {
      if (!(growth->new_root_allocated))
	{
	  *n_allocations_to_do = 1;
	}
      if (!(growth->root_fetched))
	{
	  size_t my_row, my_start, my_length;
	  const struct bplus_node *node =
	    fetcher_status (growth->old_root_fetcher, &my_row, &my_start,
			    &my_length);
	  assert (node == NULL);
	  /* Otherwise, we fail to growth_advance()
	     earlier. */
	  /* Push a fetch. */
	  size_t i = *n_fetches_to_do;
	  assert (i == 0);
	  if (i >= start_fetch_to_do
	      && i - start_fetch_to_do < max_fetches_to_do)
	    {
	      const size_t index = i - start_fetch_to_do;
	      fetch_rows[index] = my_row;
	      fetch_starts[index] = my_start;
	      fetch_lengths[index] = my_length;
	    }
	  *n_fetches_to_do = i + 1;
	}
    }
  if (*done)
    {
      assert (*n_fetches_to_do == 0);
      assert (*n_allocations_to_do == 0);
      assert (*n_stores_to_do == 0);
    }
  else
    {
      assert (*n_fetches_to_do + *n_allocations_to_do + *n_stores_to_do != 0);
    }
}

#endif /* H_BPLUS_GROWTH_INCLUDED */
