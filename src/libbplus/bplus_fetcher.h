#ifndef H_BPLUS_FETCHER_INCLUDED
# define H_BPLUS_FETCHER_INCLUDED

# include <bplus.h>

# include <stdlib.h>
# include <string.h>
# include <assert.h>

struct bplus_fetcher;

static struct bplus_fetcher *fetcher_alloc (struct bplus_tree *tree);

static void fetcher_free (struct bplus_fetcher *fetcher);

static void fetcher_setup (struct bplus_fetcher *fetcher, uint32_t node_id);

static const struct bplus_node *fetcher_status (struct bplus_fetcher *fetcher,
						size_t *row_to_fetch,
						size_t *start,
						size_t *length);

static void fetcher_data (struct bplus_fetcher *fetcher, size_t row,
			  size_t start, size_t length, const uint32_t * data);

static inline uint32_t _fetcher_id (struct bplus_fetcher *fetcher);

struct bplus_fetcher
{
  struct bplus_tree *tree;
  size_t order;
  uint32_t request;
  struct bplus_node dest;
  int fetched;
};

static struct bplus_fetcher *
fetcher_alloc (struct bplus_tree *tree)
{
  struct bplus_fetcher *fetcher = malloc (sizeof (struct bplus_fetcher));
  if (fetcher != NULL)
    {
      fetcher->tree = tree;
      fetcher->order = bplus_tree_order (tree);
      uint32_t *keys = malloc ((fetcher->order - 1) * sizeof (uint32_t));
      uint32_t *values = malloc (fetcher->order * sizeof (uint32_t));
      if (keys == NULL || values == NULL)
	{
	  free (keys);
	  free (values);
	  free (fetcher);
	  fetcher = NULL;
	}
      else
	{
	  bplus_node_setup (&(fetcher->dest), fetcher->order, keys, values);
	  fetcher->fetched = 0;
	}
    }
  return fetcher;
}

static void
fetcher_free (struct bplus_fetcher *fetcher)
{
  if (fetcher != NULL)
    {
      free (fetcher->dest.keys);
      free (fetcher->dest.values);
    }
  free (fetcher);
}

static void
fetcher_setup (struct bplus_fetcher *fetcher, uint32_t node_id)
{
  fetcher->request = node_id;
  fetcher->fetched = 0;
  const struct bplus_node *cached =
    bplus_tree_get_cache (fetcher->tree, node_id);
  if (cached != NULL)
    {
      /* We’re lucky! */
      bplus_node_copy (fetcher->order, &(fetcher->dest), cached);
      fetcher->fetched = 1;
    }
}

static const struct bplus_node *
fetcher_status (struct bplus_fetcher *fetcher, size_t *row_to_fetch,
		size_t *start, size_t *length)
{
  if (row_to_fetch)
    {
      *row_to_fetch = 0;
    }
  if (start)
    {
      *start = 0;
    }
  if (length)
    {
      *length = 0;
    }
  const size_t order = fetcher->order;
  if (fetcher->fetched)
    {
      return &(fetcher->dest);
    }
  else
    {
      if (row_to_fetch)
	{
	  *row_to_fetch = fetcher->request;
	}
      if (start)
	{
	  *start = 0;
	}
      if (length)
	{
	  *length = 2 * order + 1;
	}
      return NULL;
    }
}

static void
fetcher_data (struct bplus_fetcher *fetcher, size_t row, size_t start,
	      size_t length, const uint32_t * data)
{
  const size_t order = fetcher->order;
  if (fetcher->fetched == 0
      && row == fetcher->request && start == 0 && length == 2 * order + 1)
    {
      fetcher->dest.n_entries = 0;
      uint32_t flags = data[2 * order];
      uint32_t is_leaf_mask = ((uint32_t) 1) << 31;
      fetcher->dest.is_leaf = ((flags & is_leaf_mask) != 0);
      fetcher->dest.parent_node = data[2 * order - 1];
      size_t i = 0;
      for (i = 0; i + 1 < order && data[i] != ((uint32_t) (-1)); i++)
	{
	  fetcher->dest.keys[i] = data[i];
	  fetcher->dest.values[i] = data[i + (order - 1)];
	}
      if (fetcher->dest.is_leaf)
	{
	  /* "next leaf" */
	  fetcher->dest.values[order - 1] = data[2 * order - 2];
	}
      else
	{
	  /* the child strictly after the last key */
	  fetcher->dest.values[i] = data[i + (order - 1)];
	}
      fetcher->dest.n_entries = i;
      bplus_tree_cache (fetcher->tree, fetcher->request, &(fetcher->dest));
      fetcher->fetched = 1;
    }
}

static inline uint32_t
_fetcher_id (struct bplus_fetcher *fetcher)
{
  return fetcher->request;
}

#endif /* H_BPLUS_FETCHER_INCLUDED */
