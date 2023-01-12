#ifndef H_BPLUS_FETCH_INCLUDED
# define H_BPLUS_FETCH_INCLUDED

# include <bplus.h>

# include <stdlib.h>
# include <string.h>
# include <assert.h>

static inline
  int fetch (struct bplus_tree *tree, bplus_fetch_cb fetch,
	     void *fetch_context, uint32_t id, struct bplus_node *node);

# include "bplus_fetcher.h"
# include "bplus_tree.h"
# include "bplus_node.h"
# include <errno.h>

static int
fetch (struct bplus_tree *tree, bplus_fetch_cb fetch, void *fetch_context,
       uint32_t id, struct bplus_node *node)
{
  struct bplus_fetcher *fetcher = fetcher_alloc (tree);
  if (fetcher == NULL)
    {
      return ENOMEM;
    }
  fetcher_setup (fetcher, id);
  size_t row_to_fetch, start, length;
  const struct bplus_node *fetched = NULL;
  const size_t order = tree_order (tree);
  uint32_t *user_data = malloc ((2 * order + 1) * sizeof (uint32_t));
  if (user_data == NULL)
    {
      fetcher_free (fetcher);
      return ENOMEM;
    }
  do
    {
      fetched = fetcher_status (fetcher, &row_to_fetch, &start, &length);
      if (fetched == NULL)
	{
	  /* Ask the user to fetch row_to_fetch, starting at start,
	     for length elements. */
	  assert (length <= 2 * order + 1);
	  size_t n_fetched = 0;
	  int error =
	    fetch (fetch_context, row_to_fetch, start, length, &n_fetched,
		   user_data);
	  if (error != 0)
	    {
	      free (user_data);
	      fetcher_free (fetcher);
	      return error;
	    }
	  fetcher_data (fetcher, row_to_fetch, start, n_fetched, user_data);
	}
    }
  while (fetched == NULL);
  node_copy (order, node, fetched);
  free (user_data);
  fetcher_free (fetcher);
  return 0;
}

#endif /* H_BPLUS_FETCH_INCLUDED */
