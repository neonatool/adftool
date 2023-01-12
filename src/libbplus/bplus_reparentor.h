#ifndef H_BPLUS_REPARENTOR_INCLUDED
# define H_BPLUS_REPARENTOR_INCLUDED

# include <bplus.h>
# include "bplus_fetcher.h"

# include <stdlib.h>
# include <string.h>
# include <assert.h>

/* The reparentor will get a node, and make sure that all of its
   children recognize it as parent. */

struct bplus_reparentor;

static struct bplus_reparentor *reparentor_alloc (struct bplus_tree *tree);

static void reparentor_free (struct bplus_reparentor *reparentor);

  /* This works even if the node is a leaf, in which case nothing will
     be done. */

static void reparentor_setup (struct bplus_reparentor *reparentor,
			      uint32_t id, const struct bplus_node *node);

static void reparentor_status (const struct bplus_reparentor *reparentor,
			       int *done,
			       /* The reparentor will expect you to
			          fetch nodes: */
			       size_t *n_fetches_to_do,
			       size_t start_fetch_to_do,
			       size_t max_fetches_to_do,
			       size_t *fetch_rows,
			       size_t *fetch_starts, size_t *fetch_lengths,
			       /* The reparentor will expect you to
			          propagate the updates: */
			       size_t *n_stores_to_do,
			       size_t start_store_to_do,
			       size_t max_stores_to_do,
			       size_t *store_rows,
			       size_t *store_starts,
			       size_t *store_lengths,
			       const uint32_t ** stores);

static void reparentor_data (struct bplus_reparentor *reparentor,
			     size_t row, size_t start, size_t length,
			     const uint32_t * data);

static void reparentor_updated (struct bplus_reparentor *reparentor,
				size_t row);

# define ITEM_TO_BE_FETCHED 0
# define ITEM_TO_BE_STORED 1
# define ITEM_DONE 2

struct bplus_reparentor
{
  size_t order;
  struct bplus_tree *tree;
  size_t n_children;
  uint32_t desired_parent;

  /* FIXME: proceed with batches to avoid order² space complexity? */

  /* order of them allocated, but only the first n_children of them
     are actually used. */
  struct bplus_fetcher **fetchers;
  int *status;			/* Whether the fetcher has already been updated. */
  uint32_t *updates;		/* the full rows to update, chained in
				   the same array for easier
				   allocation. */
};

struct bplus_reparentor *
reparentor_alloc (struct bplus_tree *tree)
{
  struct bplus_reparentor *reparentor =
    malloc (sizeof (struct bplus_reparentor));
  if (reparentor != NULL)
    {
      reparentor->order = bplus_tree_order (tree);
      reparentor->tree = tree;
      size_t max_children = reparentor->order;
      reparentor->fetchers =
	malloc (max_children * sizeof (struct bplus_fetcher *));
      reparentor->status = malloc (max_children * sizeof (int));
      reparentor->updates =
	malloc (max_children * (2 * reparentor->order + 1) *
		sizeof (uint32_t *));
      if (reparentor->fetchers == NULL || reparentor->status == NULL
	  || reparentor->updates == NULL)
	{
	  free (reparentor->fetchers);
	  free (reparentor->status);
	  free (reparentor->updates);
	  free (reparentor);
	  return NULL;
	}
      for (size_t i = 0; i < max_children; i++)
	{
	  reparentor->fetchers[i] = fetcher_alloc (tree);
	  if (reparentor->fetchers[i] == NULL)
	    {
	      for (size_t j = 0; j <= i; j++)
		{
		  fetcher_free (reparentor->fetchers[j]);
		}
	      free (reparentor->fetchers);
	      free (reparentor->status);
	      free (reparentor->updates);
	      free (reparentor);
	      return NULL;
	    }
	}
    }
  return reparentor;
}

static void
reparentor_free (struct bplus_reparentor *reparentor)
{
  if (reparentor != NULL)
    {
      for (size_t i = 0; i < reparentor->order; i++)
	{
	  fetcher_free (reparentor->fetchers[i]);
	}
      free (reparentor->fetchers);
      free (reparentor->status);
      free (reparentor->updates);
    }
  free (reparentor);
}

static void
reparentor_advance (struct bplus_reparentor *reparentor)
{
  const size_t ncol = 2 * reparentor->order + 1;
  for (size_t i = 0; i < reparentor->n_children; i++)
    {
      switch (reparentor->status[i])
	{
	case ITEM_TO_BE_FETCHED:
	  if (1)
	    {
	      const struct bplus_node *node =
		fetcher_status (reparentor->fetchers[i], NULL, NULL, NULL);
	      if (node != NULL)
		{
		  const size_t order = reparentor->order;
		  /* Export the update. First, the order - 1 keys. */
		  for (size_t i_key = 0; i_key < order - 1; i_key++)
		    {
		      reparentor->updates[i * ncol + i_key] =
			((uint32_t) (-1));
		      if (i_key < node->n_entries)
			{
			  reparentor->updates[i * ncol + i_key] =
			    node->keys[i_key];
			}
		    }
		  /* Next, the values. */
		  for (size_t i_value = 0; i_value < order - 1; i_value++)
		    {
		      reparentor->updates[i * ncol + order - 1 + i_value] = 0;
		      if (i_value < node->n_entries)
			{
			  reparentor->updates[i * ncol + order - 1 +
					      i_value] =
			    node->values[i_value];
			}
		    }
		  reparentor->updates[i * ncol + 2 * order - 2] = 0;
		  /* The last value, or the next leaf */
		  if (node->is_leaf)
		    {
		      reparentor->updates[i * ncol + 2 * order - 2] =
			node->values[order - 1];
		    }
		  else
		    {
		      reparentor->updates[i * ncol + order - 1 +
					  node->n_entries] =
			node->values[node->n_entries];
		    }
		  /* Next, the parent. */
		  reparentor->updates[i * ncol + 2 * order - 1] =
		    reparentor->desired_parent;
		  /* Finally, the flags. */
		  reparentor->updates[i * ncol + 2 * order] = 0;
		  if (node->is_leaf)
		    {
		      const uint32_t one = 1;
		      const uint32_t flag = one << 31;
		      reparentor->updates[i * ncol + 2 * order] = flag;
		    }
		  reparentor->status[i] = ITEM_TO_BE_STORED;
		  if (reparentor->desired_parent == node->parent_node)
		    {
		      /* Skip this. */
		      reparentor->status[i] = ITEM_DONE;
		    }
		}
	    }
	  break;
	default:
	  break;
	}
    }
}

static void
reparentor_setup (struct bplus_reparentor *reparentor, uint32_t id,
		  const struct bplus_node *node)
{
  reparentor->n_children = 0;
  reparentor->desired_parent = id;
  if (!node->is_leaf)
    {
      reparentor->n_children = node->n_entries + 1;
      for (size_t i = 0; i < reparentor->n_children; i++)
	{
	  fetcher_setup (reparentor->fetchers[i], node->values[i]);
	  reparentor->status[i] = ITEM_TO_BE_FETCHED;
	}
    }
  reparentor_advance (reparentor);
}

static void
reparentor_status (const struct bplus_reparentor *reparentor,
		   int *done,
		   size_t *n_fetches_to_do,
		   size_t start_fetch_to_do,
		   size_t max_fetches_to_do,
		   size_t *fetch_rows,
		   size_t *fetch_starts, size_t *fetch_lengths,
		   /* The reparentor will expect you to
		      propagate the updates: */
		   size_t *n_stores_to_do,
		   size_t start_store_to_do,
		   size_t max_stores_to_do,
		   size_t *store_rows,
		   size_t *store_starts,
		   size_t *store_lengths, const uint32_t ** stores)
{
  const size_t order = reparentor->order;
  const size_t ncol = 2 * order + 1;
  *n_fetches_to_do = 0;
  *n_stores_to_do = 0;
  *done = 1;
  for (size_t i = 0; i < reparentor->n_children; i++)
    {
      size_t row, start, length;
      const struct bplus_node *fetched =
	fetcher_status (reparentor->fetchers[i], &row, &start, &length);
      if (fetched == NULL)
	{
	  /* Push the fetch request. */
	  size_t j = *n_fetches_to_do;
	  if (j >= start_fetch_to_do
	      && j - start_fetch_to_do < max_fetches_to_do)
	    {
	      fetch_rows[j] = row;
	      fetch_starts[j] = start;
	      fetch_lengths[j] = length;
	    }
	  *n_fetches_to_do = j + 1;
	  *done = 0;
	}
      else if (reparentor->status[i] == ITEM_TO_BE_STORED)
	{
	  /* Push the store request. */
	  size_t j = *n_stores_to_do;
	  if (j >= start_store_to_do
	      && j - start_store_to_do < max_stores_to_do)
	    {
	      store_rows[j] = _fetcher_id (reparentor->fetchers[i]);
	      store_starts[j] = 0;
	      store_lengths[j] = ncol;
	      stores[j] = &(reparentor->updates[i * ncol]);
	    }
	  *n_stores_to_do = j + 1;
	  *done = 0;
	}
    }
}

static void
reparentor_data (struct bplus_reparentor *reparentor, size_t row,
		 size_t start, size_t length, const uint32_t * data)
{
  for (size_t i = 0; i < reparentor->n_children; i++)
    {
      if (reparentor->status[i] == ITEM_TO_BE_FETCHED)
	{
	  fetcher_data (reparentor->fetchers[i], row, start, length, data);
	}
    }
  reparentor_advance (reparentor);
}

static void
reparentor_updated (struct bplus_reparentor *reparentor, size_t row)
{
  const size_t order = reparentor->order;
  for (size_t i = 0; i < reparentor->n_children; i++)
    {
      if (reparentor->status[i] == ITEM_TO_BE_STORED
	  && row == _fetcher_id (reparentor->fetchers[i]))
	{
	  reparentor->status[i] = ITEM_DONE;
	  const uint32_t *update =
	    &(reparentor->updates[i * (2 * order + 1)]);
	  size_t n_entries = 0;
	  for (n_entries = 0;
	       n_entries < order - 1
	       && update[n_entries] != ((uint32_t) (-1)); n_entries++)
	    ;
	  uint32_t mask = ((uint32_t) ((uint32_t) 1) << 31);
	  uint32_t masked = update[2 * order] & mask;
	  const struct bplus_node finalized = {
	    .n_entries = n_entries,
	    .keys = (uint32_t *) update,
	    .values = (uint32_t *) & (update[order - 1]),
	    .is_leaf = (masked != 0),
	    .parent_node = update[2 * order - 1]
	  };
	  assert (finalized.parent_node == reparentor->desired_parent);
	  bplus_tree_cache (reparentor->tree,
			    _fetcher_id (reparentor->fetchers[i]),
			    &finalized);
	}
    }
  reparentor_advance (reparentor);
}

#endif /* H_BPLUS_REPARENTOR_INCLUDED */
