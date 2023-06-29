#ifndef H_BPLUS_FIND_INCLUDED
# define H_BPLUS_FIND_INCLUDED

# include <bplus.h>

# include <stdlib.h>
# include <string.h>
# include <assert.h>
# include <errno.h>
# include <stdbool.h>

static inline
  int find (struct bplus_tree *tree, bplus_fetch_cb fetch,
	    void *fetch_context, bplus_compare_cb compare,
	    void *compare_context, bplus_iterator_cb iterator,
	    void *iterator_context, const struct bplus_key *key);

# include "bplus_finder.h"
# include "bplus_tree.h"
# include "bplus_node.h"
# include "bplus_fetch.h"

static int
_find_range (struct bplus_tree *tree, bplus_fetch_cb fetch_impl,
	     void *fetch_context, bplus_compare_cb compare,
	     void *compare_context, struct bplus_range *range,
	     const struct bplus_key *key)
{
  const size_t order = tree_order (tree);
  int ret = 0;
  struct bplus_node root;
  uint32_t *root_keys = malloc ((order - 1) * sizeof (uint32_t));
  uint32_t *root_values = malloc (order * sizeof (uint32_t));
  if (root_keys == NULL || root_values == NULL)
    {
      ret = ENOMEM;
      goto cleanup_root_node;
    }
  node_setup (&root, order, root_keys, root_values);
  int error = fetch (tree, fetch_impl, fetch_context, 0, &root);
  if (error != 0)
    {
      ret = error;
      goto cleanup_root_node;
    }
  struct bplus_finder *finder = finder_alloc (tree);
  if (finder == NULL)
    {
      ret = ENOMEM;
      goto cleanup_root_node;
    }
  finder_setup (finder, key, 0, &root);
  int finder_done;
  size_t n_fetch_requests;
  size_t start_fetch_request = 0;
  size_t max_fetch_requests = 256;
  size_t fetch_rows[256];
  size_t fetch_starts[256];
  size_t fetch_lengths[256];
  size_t n_compare_requests;
  size_t start_compare_request = 0;
  size_t max_compare_requests = 256;
  struct bplus_key as[256];
  struct bplus_key bs[256];
  uint32_t *user_data = malloc ((2 * order + 1) * sizeof (uint32_t));
  int user_compared;
  if (user_data == NULL)
    {
      ret = ENOMEM;
      goto cleanup_finder;
    }
  do
    {
      finder_status (finder, &finder_done, range, &n_fetch_requests,
		     start_fetch_request, max_fetch_requests, fetch_rows,
		     fetch_starts, fetch_lengths, &n_compare_requests,
		     start_compare_request, max_compare_requests, as, bs);
      for (size_t i = 0; i < n_fetch_requests && i < max_fetch_requests; i++)
	{
	  assert (fetch_lengths[i] <= 2 * order + 1);
	  size_t n_fetched = 0;
	  int error =
	    fetch_impl (fetch_context, fetch_rows[i], fetch_starts[i],
			fetch_lengths[i], &n_fetched, user_data);
	  if (error)
	    {
	      ret = error;
	      goto cleanup_user_data;
	    }
	  finder_data (finder, fetch_rows[i], fetch_starts[i], n_fetched,
		       user_data);
	}
      for (size_t i = 0; i < n_compare_requests && i < max_compare_requests;
	   i++)
	{
	  int error =
	    compare (compare_context, &(as[i]), &(bs[i]), &user_compared);
	  if (error)
	    {
	      ret = error;
	      goto cleanup_user_data;
	    }
	  finder_compared (finder, &(as[i]), &(bs[i]), user_compared);
	}
    }
  while (!finder_done);
cleanup_user_data:
  free (user_data);
cleanup_finder:
  bplus_finder_free (finder);
cleanup_root_node:
  free (root_keys);
  free (root_values);
  return ret;
}

static inline int
find (struct bplus_tree *tree, bplus_fetch_cb fetch_impl, void *fetch_context,
      bplus_compare_cb compare, void *compare_context,
      bplus_iterator_cb iterator, void *iterator_context,
      const struct bplus_key *key)
{
  const size_t order = tree_order (tree);
  struct bplus_range *range = range_alloc (tree);
  if (range == NULL)
    {
      return ENOMEM;
    }
  int ret =
    _find_range (tree, fetch_impl, fetch_context, compare, compare_context,
		 range, key);
  if (ret == 0)
    {
      size_t max_elements = 1;
      struct bplus_key *keys =
	malloc (max_elements * sizeof (struct bplus_key));
      uint32_t *values = malloc (max_elements * sizeof (uint32_t));
      if (keys == NULL || values == NULL)
	{
	  ret = ENOMEM;
	  goto cleanup_keys_values;
	}
      int has_next;
      size_t n_fetch_requests;
      size_t start_fetch_request = 0;
      size_t max_fetch_requests = 1;
      size_t fetch_rows[1];
      size_t fetch_starts[1];
      size_t fetch_lengths[1];
      uint32_t *user_data = malloc ((2 * order + 1) * sizeof (uint32_t));
      if (user_data == NULL)
	{
	  ret = ENOMEM;
	  goto cleanup_user_data;
	}
      while (true)
	{
	  size_t n_total =
	    range_get (range, 0, max_elements, keys, values, &has_next,
		       &n_fetch_requests, start_fetch_request,
		       max_fetch_requests, fetch_rows, fetch_starts,
		       fetch_lengths);
	  if (n_fetch_requests != 0)
	    {
	      assert (fetch_lengths[0] <= 2 * order + 1);
	      size_t n_fetched;
	      int error =
		fetch_impl (fetch_context, fetch_rows[0], fetch_starts[0],
			    fetch_lengths[0], &n_fetched, user_data);
	      if (error)
		{
		  ret = error;
		  goto cleanup_user_data;
		}
	      range_data (range, fetch_rows[0], fetch_starts[0], n_fetched,
			  user_data);
	    }
	  else
	    {
	      if (n_total > max_elements)
		{
		  max_elements = n_total;
		  keys =
		    realloc (keys, max_elements * sizeof (struct bplus_key));
		  values = realloc (values, max_elements * sizeof (uint32_t));
		  if (keys == NULL || values == NULL)
		    {
		      ret = ENOMEM;
		      goto cleanup_user_data;
		    }
		}
	      else
		{
		  int error = 0;
		  if (n_total != 0)
		    {
		      error =
			iterator (iterator_context, n_total, keys, values);
		    }
		  if (error)
		    {
		      ret = error;
		      goto cleanup_user_data;
		    }
		  if (has_next)
		    {
		      int range_cant_advance = range_next (range);
		      if (range_cant_advance)
			{
			  /* Impossible, it reports 0 to fetch */
			  assert (0);
			}
		    }
		  else
		    {
		      ret = iterator (iterator_context, 0, NULL, NULL);
		      goto cleanup_user_data;
		    }
		}
	    }
	}
    cleanup_user_data:
      free (user_data);
    cleanup_keys_values:
      free (keys);
      free (values);
    }
  range_free (range);
  return ret;
}

#endif /* H_BPLUS_FIND_INCLUDED */
