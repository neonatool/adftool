#ifndef H_BPLUS_FINDER_INCLUDED
# define H_BPLUS_FINDER_INCLUDED

# include <bplus.h>

# include <stdlib.h>
# include <string.h>

  /* The finder process is a recursive explorer. It starts at the
     root, and advances both a "first" and a "last" leg until it
     touches leaves. The value produced by the finder is a range. This
     range contains all the values in the tree that compare equal to
     the search key. */

struct bplus_finder;

static inline
  struct bplus_finder *bplus_finder_alloc (struct bplus_tree *tree);

static inline void bplus_finder_free (struct bplus_finder *finder);

static inline
  void bplus_finder_setup (struct bplus_finder *finder,
			   const struct bplus_key *search_key,
			   uint32_t root_id, const struct bplus_node *root);

static inline
  void bplus_finder_status (const struct bplus_finder *finder,
			    int *done,
			    struct bplus_range *result,
			    size_t *n_fetch_requests,
			    size_t start_fetch_request,
			    size_t max_fetch_requests,
			    size_t *fetch_rows,
			    size_t *fetch_starts,
			    size_t *fetch_lengths,
			    size_t *n_compare_requests,
			    size_t start_compare_request,
			    size_t max_compare_requests,
			    struct bplus_key *as, struct bplus_key *bs);

# include "bplus_analyzer.h"
# include "bplus_explorer.h"
# include "bplus_tree.h"
# include "bplus_range.h"
# include "bplus_key.h"

struct bplus_finder
{
  size_t order;
  struct bplus_key pivot;

  struct bplus_explorer *step;

  int leaf_step_started;
  uint32_t first_leaf_id;
  uint32_t last_leaf_id;
  struct bplus_node first_leaf;
  struct bplus_node last_leaf;
  struct bplus_analyzer first_leaf_analyzer;
  int separate_last_leaf_analyzer;
  struct bplus_analyzer last_leaf_analyzer;
};

static inline struct bplus_finder *
finder_alloc (struct bplus_tree *tree)
{
  struct bplus_finder *finder = malloc (sizeof (struct bplus_finder));
  if (finder != NULL)
    {
      finder->order = tree_order (tree);
      finder->step = explorer_alloc (tree);
      finder->first_leaf.keys =
	malloc ((finder->order - 1) * sizeof (uint32_t));
      finder->first_leaf.values = malloc (finder->order * sizeof (uint32_t));
      finder->last_leaf.keys =
	malloc ((finder->order - 1) * sizeof (uint32_t));
      finder->last_leaf.values = malloc (finder->order * sizeof (uint32_t));
      int e_first =
	(analyzer_init (&(finder->first_leaf_analyzer), finder->order) != 0);
      int e_last =
	(analyzer_init (&(finder->last_leaf_analyzer), finder->order) != 0);
      if (finder->step == NULL || e_first != 0 || e_last != 0
	  || finder->first_leaf.keys == NULL
	  || finder->first_leaf.values == NULL
	  || finder->last_leaf.keys == NULL
	  || finder->last_leaf.values == NULL)
	{
	  free (finder->first_leaf.keys);
	  free (finder->first_leaf.values);
	  free (finder->last_leaf.keys);
	  free (finder->last_leaf.values);
	  explorer_free (finder->step);
	  if (e_first)
	    {
	      analyzer_deinit (&(finder->first_leaf_analyzer));
	    }
	  if (e_last)
	    {
	      analyzer_deinit (&(finder->last_leaf_analyzer));
	    }
	  free (finder);
	  finder = NULL;
	}
    }
  return finder;
}

static inline void
finder_free (struct bplus_finder *finder)
{
  if (finder != NULL)
    {
      free (finder->first_leaf.keys);
      free (finder->first_leaf.values);
      free (finder->last_leaf.keys);
      free (finder->last_leaf.values);
      explorer_free (finder->step);
      analyzer_deinit (&(finder->first_leaf_analyzer));
      analyzer_deinit (&(finder->last_leaf_analyzer));
    }
  free (finder);
}

static inline void
finder_advance (struct bplus_finder *finder)
{
  struct bplus_explorer_status status;
  status.to_fetch.start = 0;
  status.to_fetch.max = 0;
  status.to_fetch.rows_to_fetch = NULL;
  status.to_fetch.row_starts = NULL;
  status.to_fetch.row_lengths = NULL;
  status.to_compare.start = 0;
  status.to_compare.max = 0;
  status.to_compare.as = NULL;
  status.to_compare.bs = NULL;
  if (!(finder->leaf_step_started))
    {
      explorer_status (finder->step, &status);
      if (status.done && node_is_leaf (status.first_node)
	  && node_is_leaf (status.last_node))
	{
	  /* Start the last step, which is to locate the key in the
	     leaves. */
	  analyzer_setup (&(finder->first_leaf_analyzer),
			  status.first_node->n_entries,
			  status.first_node->keys, &(finder->pivot));
	  if (status.first_id != status.last_id)
	    {
	      analyzer_setup (&(finder->last_leaf_analyzer),
			      status.last_node->n_entries,
			      status.last_node->keys, &(finder->pivot));
	    }
	  finder->separate_last_leaf_analyzer =
	    (status.first_id != status.last_id);
	  finder->leaf_step_started = 1;
	  finder->first_leaf_id = status.first_id;
	  finder->last_leaf_id = status.last_id;
	  node_copy (finder->order, &(finder->first_leaf), status.first_node);
	  node_copy (finder->order, &(finder->last_leaf), status.last_node);
	}
      else if (status.done
	       && (!node_is_leaf (status.first_node)
		   || !node_is_leaf (status.last_node)))
	{
	  /* If either node is not a leaf, neither are. */
	  assert (!node_is_leaf (status.first_node));
	  assert (!node_is_leaf (status.last_node));
	  int setup_error =
	    explorer_setup (finder->step, &(finder->pivot), status.first_id,
			    status.first_node, status.last_id,
			    status.last_node);
	  assert (setup_error == 0);
	  finder_advance (finder);
	}
    }
}

static inline void
finder_setup (struct bplus_finder *finder,
	      const struct bplus_key *search_key,
	      uint32_t root_id, const struct bplus_node *root)
{
  memcpy (&(finder->pivot), search_key, sizeof (struct bplus_key));
  if (root->is_leaf)
    {
      finder->leaf_step_started = 1;
      finder->first_leaf_id = root_id;
      finder->last_leaf_id = root_id;
      node_copy (finder->order, &(finder->first_leaf), root);
      node_copy (finder->order, &(finder->last_leaf), root);
      finder->separate_last_leaf_analyzer = 0;
      analyzer_setup (&(finder->first_leaf_analyzer), root->n_entries,
		      root->keys, &(finder->pivot));
    }
  else
    {
      if (explorer_setup
	  (finder->step, search_key, root_id, root, root_id, root) != 0)
	{
	  abort ();
	}
      finder->leaf_step_started = 0;
      finder_advance (finder);
    }
}

static inline void
finder_status (const struct bplus_finder *finder,
	       int *done,
	       struct bplus_range *result,
	       size_t *n_fetch_requests,
	       size_t start_fetch_requests,
	       size_t max_fetch_requests,
	       size_t *fetch_rows,
	       size_t *fetch_starts,
	       size_t *fetch_lengths,
	       size_t *n_compare_requests,
	       size_t start_compare_requests,
	       size_t max_compare_requests,
	       struct bplus_key *as, struct bplus_key *bs)
{
  *done = 0;
  if (finder->leaf_step_started)
    {
      /* Now there are only final compare requests. */
      *n_fetch_requests = 0;
      *n_compare_requests = 0;
      int first_done, last_done;
      int first_found, last_found;
      size_t first_index, last_index;
      struct bplus_key first_a, first_b, last_a, last_b;
      analyzer_match (&(finder->first_leaf_analyzer), BPLUS_ANALYZER_FIRST,
		      &first_done, &first_found, &first_index, &first_a,
		      &first_b);
      const struct bplus_analyzer *last_analyzer =
	&(finder->first_leaf_analyzer);
      if (finder->separate_last_leaf_analyzer)
	{
	  last_analyzer = &(finder->last_leaf_analyzer);
	}
      analyzer_match (last_analyzer, BPLUS_ANALYZER_LAST, &last_done,
		      &last_found, &last_index, &last_a, &last_b);
      if (first_done && last_done)
	{
	  *done = 1;
	  *n_compare_requests = 0;
	  int setup_error = range_setup (result, finder->first_leaf_id,
					 &(finder->first_leaf),
					 first_index,
					 finder->last_leaf_id,
					 &(finder->last_leaf),
					 last_index);
	  assert (setup_error == 0);
	}
      else
	{
	  if (!first_done)
	    {
	      /* Push first_a, first_b in the queue of comparisons to make. */
	      size_t i = *n_compare_requests;
	      if (i >= start_compare_requests
		  && (i - start_compare_requests < max_compare_requests))
		{
		  memcpy (&(as[i - start_compare_requests]), &(first_a),
			  sizeof (struct bplus_key));
		  memcpy (&(bs[i - start_compare_requests]), &(first_b),
			  sizeof (struct bplus_key));
		}
	      *n_compare_requests = i + 1;
	    }
	  if (!last_done
	      && (first_done
		  || !key_identical (&first_a, &last_a)
		  || !key_identical (&first_b, &last_b)))
	    {
	      /* Push last_a, last_b in the queue of comparisons to make. */
	      size_t i = *n_compare_requests;
	      if (i >= start_compare_requests
		  && i - start_compare_requests < max_compare_requests)
		{
		  memcpy (&(as[i - start_compare_requests]), &(last_a),
			  sizeof (struct bplus_key));
		  memcpy (&(bs[i - start_compare_requests]), &(last_b),
			  sizeof (struct bplus_key));
		}
	      *n_compare_requests = i + 1;
	    }
	}
    }
  else
    {
      struct bplus_explorer_status status;
      status.to_fetch.start = start_fetch_requests;
      status.to_fetch.max = max_fetch_requests;
      status.to_fetch.rows_to_fetch = fetch_rows;
      status.to_fetch.row_starts = fetch_starts;
      status.to_fetch.row_lengths = fetch_lengths;
      status.to_compare.start = start_compare_requests;
      status.to_compare.max = max_compare_requests;
      status.to_compare.as = as;
      status.to_compare.bs = bs;
      explorer_status (finder->step, &status);
      assert (!status.done);
      *n_fetch_requests = status.to_fetch.n;
      *n_compare_requests = status.to_compare.n;
    }
}

static inline void
finder_data (struct bplus_finder *finder, size_t row, size_t start,
	     size_t length, const uint32_t * data)
{
  if (!(finder->leaf_step_started))
    {
      explorer_data (finder->step, row, start, length, data);
      finder_advance (finder);
    }
  else
    {
      assert (finder->leaf_step_started);
      /* The leaf step does not expect more data. */
    }
}

static inline void
finder_compared (struct bplus_finder *finder, const struct bplus_key *a,
		 const struct bplus_key *b, int result)
{
  if (!(finder->leaf_step_started))
    {
      explorer_compared (finder->step, a, b, result);
      finder_advance (finder);
    }
  else
    {
      assert (finder->leaf_step_started);
      analyzer_result (&(finder->first_leaf_analyzer), a, b, result);
      if (finder->separate_last_leaf_analyzer)
	{
	  analyzer_result (&(finder->last_leaf_analyzer), a, b, result);
	}
    }
}

#endif /* not H_BPLUS_FINDER_INCLUDED */
