#ifndef H_BPLUS_INSERT_INCLUDED
# define H_BPLUS_INSERT_INCLUDED

# include <bplus.h>

# include <stdlib.h>
# include <string.h>
# include <assert.h>
# include <errno.h>
# include <stdbool.h>

static inline int insert (struct bplus_tree *tree, bplus_fetch_cb fetch,
			  void *fetch_context, bplus_compare_cb compare,
			  void *compare_context, bplus_allocate_cb allocate,
			  void *allocate_context, bplus_update_cb update,
			  void *update_context, bplus_decision_cb decide,
			  void *decide_context, const struct bplus_key *key);

# include "bplus_find.h"
# include "bplus_tree.h"
# include "bplus_node.h"
# include "bplus_fetch.h"
# include "bplus_range.h"
# include "bplus_insertion.h"

static inline
  int _find_range (struct bplus_tree *tree, bplus_fetch_cb fetch,
		   void *fetch_context, bplus_compare_cb compare,
		   void *compare_context, struct bplus_range *range,
		   const struct bplus_key *key);

static inline int
insert (struct bplus_tree *tree, bplus_fetch_cb fetch,
	void *fetch_context, bplus_compare_cb compare,
	void *compare_context, bplus_allocate_cb allocate,
	void *allocate_context, bplus_update_cb update,
	void *update_context, bplus_decision_cb decide,
	void *decide_context, const struct bplus_key *key)
{
  const size_t order = tree_order (tree);
  struct bplus_range *range = range_alloc (tree);
  if (range == NULL)
    {
      return ENOMEM;
    }
  int ret = _find_range (tree, fetch, fetch_context, compare, compare_context,
			 range, key);
  if (ret == 0)
    {
      int range_has_next;
      size_t n_load_requests;
      size_t range_n = range_get (range, 0, 0, NULL, NULL, &range_has_next,
				  &n_load_requests, 0, 0, NULL, NULL, NULL);
      const bool key_found = (range_n != 0);
      uint32_t key_id;
      int back = 0;
      int error = decide (decide_context, key_found, key, &key_id, &back);
      if (error != 0)
	{
	  ret = error;
	  goto cleanup;
	}
      struct bplus_insertion *insertion = insertion_alloc (tree);
      if (insertion == NULL)
	{
	  ret = ENOMEM;
	  goto cleanup;
	}
      insertion_setup (insertion, key_id, range, back);
      int insertion_done;
      size_t n_fetches_to_do;
      size_t start_fetch_to_do = 0;
      size_t max_fetches_to_do = 256;
      size_t fetch_rows[256];
      size_t fetch_starts[256];
      size_t fetch_lengths[256];
      size_t n_allocations_to_do;
      size_t n_stores_to_do;
      size_t start_store_to_do = 0;
      size_t max_stores_to_do = 256;
      size_t store_rows[256];
      size_t store_starts[256];
      size_t store_lengths[256];
      const uint32_t *stores[256];
      uint32_t *user_data = malloc ((2 * order + 1) * sizeof (uint32_t));
      if (user_data == NULL)
	{
	  ret = ENOMEM;
	  goto cleanup_insertion;
	}
      do
	{
	  insertion_status (insertion, &insertion_done,
			    &n_fetches_to_do, start_fetch_to_do,
			    max_fetches_to_do, fetch_rows, fetch_starts,
			    fetch_lengths, &n_allocations_to_do,
			    &n_stores_to_do, start_store_to_do,
			    max_stores_to_do, store_rows, store_starts,
			    store_lengths, stores);
	  for (size_t i = 0; i < n_fetches_to_do && i < max_fetches_to_do;
	       i++)
	    {
	      assert (fetch_lengths[i] <= 2 * order + 1);
	      size_t n_fetched = 0;
	      int error =
		fetch (fetch_context, fetch_rows[i], fetch_starts[i],
		       fetch_lengths[i], &n_fetched, user_data);
	      if (error)
		{
		  ret = error;
		  goto cleanup_user_data;
		}
	      insertion_data (insertion, fetch_rows[i], fetch_starts[i],
			      n_fetched, user_data);
	    }
	  for (size_t i = 0; i < n_allocations_to_do; i++)
	    {
	      uint32_t new_id;
	      allocate (allocate_context, &new_id);
	      int claimed = insertion_allocated (insertion, new_id);
	      assert (claimed);
	    }
	  for (size_t i = 0; i < n_stores_to_do && i < max_stores_to_do; i++)
	    {
	      assert (store_lengths[i] <= 2 * order + 1);
	      update (update_context, store_rows[i], store_starts[i],
		      store_lengths[i], stores[i]);
	      insertion_updated (insertion, store_rows[i]);
	    }
	}
      while (!insertion_done);
    cleanup_user_data:
      free (user_data);
    cleanup_insertion:
      insertion_free (insertion);
    }
cleanup:
  range_free (range);
  return ret;
}

#endif /* H_BPLUS_INSERT_INCLUDED */
