#ifndef H_BPLUS_EXPLORER_INCLUDED
# define H_BPLUS_EXPLORER_INCLUDED

# include <bplus.h>
# include "bplus_analyzer.h"
# include "bplus_fetcher.h"

# include <stdlib.h>
# include <string.h>

/* The explorer is a process that is constructed from one or two
   nodes, that are the parents for a specific key (parent of the first
   instance and parent for the first key after the last instance). It
   will find and fetch the correct children to continue finding the
   first and last occurence of a key. This is one step in the
   recursive lookup of the range of a key, if the key spans multiple
   nodes. */

struct bplus_explorer;

static struct bplus_explorer *explorer_alloc (struct bplus_tree *tree);

static void explorer_free (struct bplus_explorer *explorer);

  /* The setup function may fail, if the nodes are leaves. In this
     case, a non-zero value is returned. */
static int explorer_setup (struct bplus_explorer *explorer,
			   const struct bplus_key *search_key,
			   uint32_t first_id,
			   const struct bplus_node *first_node,
			   uint32_t last_id,
			   const struct bplus_node *last_node);

struct bplus_fetch_request
{
  /* This part you control: */
  size_t start;
  size_t max;
  /* max elements allocated for these 3. */
  size_t *rows_to_fetch;
  size_t *row_starts;
  size_t *row_lengths;

  /* The API fills rows_to_fetch up to max with node IDs that you
     should fetch from the storage. Same for row_starts and
     row_lengths, that describe which slice of the row you have to
     fetch. */

  /* n is set to the total number of nodes that should be fetched, so
     ids_to_request is filled up to whatever is smaller: max or n -
     start. */
  size_t n;
};

struct bplus_compare_request
{
  /* This is the same format as bplus_fetch_request, except now you
     have to compare as and bs elementwise: */
  size_t start;
  size_t max;
  struct bplus_key *as;
  struct bplus_key *bs;
  size_t n;
};

struct bplus_explorer_status
{
  /* YOU HAVE TO INITIALIZE THIS STRUCT! If you don’t know what to do,
     set .to_fetch.start, .to_fetch.max, .to_compare.start and
     .to_compare.max to 0. */
  int done;
  uint32_t first_id;
  uint32_t last_id;
  const struct bplus_node *first_node;
  const struct bplus_node *last_node;
  struct bplus_fetch_request to_fetch;
  struct bplus_compare_request to_compare;
};

static void explorer_status (const struct bplus_explorer *explorer,
			     struct bplus_explorer_status *status);

static void explorer_data (struct bplus_explorer *explorer, size_t row,
			   size_t start, size_t length,
			   const uint32_t * data);

static void explorer_compared (struct bplus_explorer *explorer,
			       const struct bplus_key *a,
			       const struct bplus_key *b, int result);

struct bplus_explorer
{
  size_t order;
  struct bplus_key pivot;

  /* If true, you should use the shared analyzer. Otherwise, use
     analyze_first and analyze_last. */
  int use_shared_analyzer;

  struct bplus_analyzer shared_analyzer;
  struct bplus_analyzer analyze_first;
  struct bplus_analyzer analyze_last;

  /* In any case, the children for the first and last branches are
     copied (may be identical). */
  uint32_t *children_first;
  uint32_t *children_last;

  struct bplus_fetcher *fetch_first_child;
  struct bplus_fetcher *fetch_last_child;
};

static struct bplus_explorer *
explorer_alloc (struct bplus_tree *tree)
{
  struct bplus_explorer *explorer = malloc (sizeof (struct bplus_explorer));
  if (explorer != NULL)
    {
      explorer->order = bplus_tree_order (tree);
      int e_shared =
	(analyzer_init (&(explorer->shared_analyzer), explorer->order) != 0);
      int e_first =
	(analyzer_init (&(explorer->analyze_first), explorer->order) != 0);
      int e_last =
	(analyzer_init (&(explorer->analyze_last), explorer->order) != 0);
      explorer->children_first = malloc (explorer->order * sizeof (uint32_t));
      explorer->children_last = malloc (explorer->order * sizeof (uint32_t));
      explorer->fetch_first_child = fetcher_alloc (tree);
      explorer->fetch_last_child = fetcher_alloc (tree);
      if (e_shared != 0
	  || e_first != 0
	  || e_last != 0
	  || explorer->children_first == NULL
	  || explorer->children_last == NULL
	  || explorer->fetch_first_child == NULL
	  || explorer->fetch_last_child == NULL)
	{
	  if (e_shared)
	    {
	      analyzer_deinit (&(explorer->shared_analyzer));
	    }
	  if (e_first)
	    {
	      analyzer_deinit (&(explorer->analyze_first));
	    }
	  if (e_last)
	    {
	      analyzer_deinit (&(explorer->analyze_last));
	    }
	  free (explorer->children_first);
	  free (explorer->children_last);
	  fetcher_free (explorer->fetch_first_child);
	  fetcher_free (explorer->fetch_last_child);
	  free (explorer);
	  explorer = NULL;
	}
    }
  return explorer;
}

static void
explorer_free (struct bplus_explorer *explorer)
{
  if (explorer != NULL)
    {
      analyzer_deinit (&(explorer->shared_analyzer));
      analyzer_deinit (&(explorer->analyze_first));
      analyzer_deinit (&(explorer->analyze_last));
      free (explorer->children_first);
      free (explorer->children_last);
      fetcher_free (explorer->fetch_first_child);
      fetcher_free (explorer->fetch_last_child);
    }
  free (explorer);
}

static int
explorer_setup (struct bplus_explorer *explorer,
		const struct bplus_key *search_key, uint32_t first_id,
		const struct bplus_node *first_node, uint32_t last_id,
		const struct bplus_node *last_node)
{
  if (bplus_node_is_leaf (first_node) || bplus_node_is_leaf (last_node))
    {
      return 1;
    }
  memcpy (&(explorer->pivot), search_key, sizeof (struct bplus_key));
  explorer->use_shared_analyzer = (first_id == last_id);
  if (explorer->use_shared_analyzer)
    {
      analyzer_setup (&(explorer->shared_analyzer), first_node->n_entries,
		      first_node->keys, &(explorer->pivot));
    }
  else
    {
      analyzer_setup (&(explorer->analyze_first), first_node->n_entries,
		      first_node->keys, &(explorer->pivot));
      analyzer_setup (&(explorer->analyze_last), last_node->n_entries,
		      last_node->keys, &(explorer->pivot));
    }
  memcpy (explorer->children_first, first_node->values,
	  (first_node->n_entries + 1) * sizeof (uint32_t));
  memcpy (explorer->children_last, last_node->values,
	  (last_node->n_entries + 1) * sizeof (uint32_t));
  return 0;
}

static void
explorer_status (const struct bplus_explorer *explorer,
		 struct bplus_explorer_status *status)
{
  int first_done, last_done, first_found, last_found;
  size_t first_index, last_index;
  struct bplus_key first_a, first_b, last_a, last_b;
  status->done = 0;
  status->to_fetch.n = 0;
  status->to_compare.n = 0;
  if (explorer->use_shared_analyzer)
    {
      analyzer_match (&(explorer->shared_analyzer), BPLUS_ANALYZER_FIRST,
		      &first_done, &first_found, &first_index, &first_a,
		      &first_b);
      analyzer_match (&(explorer->shared_analyzer), BPLUS_ANALYZER_LAST,
		      &last_done, &last_found, &last_index, &last_a, &last_b);
    }
  else
    {
      analyzer_match (&(explorer->analyze_first), BPLUS_ANALYZER_FIRST,
		      &first_done, &first_found, &first_index, &first_a,
		      &first_b);
      analyzer_match (&(explorer->analyze_last), BPLUS_ANALYZER_LAST,
		      &last_done, &last_found, &last_index, &last_a, &last_b);
    }
  if (first_done && last_done)
    {
      /* Oh, so now we’re fetching. */
      /* Both fetcher must have been set up during the last evaluation
         of explorer_compared. */
      size_t first_row_to_fetch, last_row_to_fetch;
      size_t first_start, last_start;
      size_t first_length, last_length;
      const struct bplus_node *first_fetched =
	fetcher_status (explorer->fetch_first_child, &first_row_to_fetch,
			&first_start, &first_length);
      const struct bplus_node *last_fetched =
	fetcher_status (explorer->fetch_last_child, &last_row_to_fetch,
			&last_start, &last_length);
      if (first_fetched != NULL && last_fetched != NULL)
	{
	  status->done = 1;
	  status->first_id = explorer->children_first[first_index];
	  status->last_id = explorer->children_last[last_index];
	  status->first_node = first_fetched;
	  status->last_node = last_fetched;
	}
      else
	{
	  /* There are fetching requests to make. */
	  if (first_fetched == NULL)
	    {
	      if (status->to_fetch.n >= status->to_fetch.start
		  && status->to_fetch.n <
		  status->to_fetch.start + status->to_fetch.max)
		{
		  size_t i = status->to_fetch.n - status->to_fetch.start;
		  status->to_fetch.rows_to_fetch[i] = first_row_to_fetch;
		  status->to_fetch.row_starts[i] = first_start;
		  status->to_fetch.row_lengths[i] = first_length;
		}
	      status->to_fetch.n++;
	    }
	  if (last_fetched == NULL
	      /* Do not duplicate the fetch! */
	      && (first_fetched != NULL
		  || first_row_to_fetch != last_row_to_fetch
		  || first_start != last_start
		  || first_length != last_length))
	    {
	      if (status->to_fetch.n >= status->to_fetch.start
		  && status->to_fetch.n <
		  status->to_fetch.start + status->to_fetch.max)
		{
		  size_t i = status->to_fetch.n - status->to_fetch.start;
		  status->to_fetch.rows_to_fetch[i] = last_row_to_fetch;
		  status->to_fetch.row_starts[i] = last_start;
		  status->to_fetch.row_lengths[i] = last_length;
		}
	      status->to_fetch.n++;
	    }
	}
    }
  else
    {
      /* There are comparisons to be made. */
      if (!first_done)
	{
	  if (status->to_compare.n >= status->to_compare.start
	      && status->to_compare.n <
	      status->to_compare.start + status->to_compare.max)
	    {
	      size_t i = status->to_compare.n - status->to_compare.start;
	      memcpy (&(status->to_compare.as[i]), &first_a,
		      sizeof (struct bplus_key));
	      memcpy (&(status->to_compare.bs[i]), &first_b,
		      sizeof (struct bplus_key));
	    }
	  status->to_compare.n++;
	}
      if (!last_done
	  && (first_done
	      || !bplus_key_identical (&first_a, &last_a)
	      || !bplus_key_identical (&first_b, &last_b)))
	{
	  if (status->to_compare.n >= status->to_compare.start
	      && status->to_compare.n <
	      status->to_compare.start + status->to_compare.max)
	    {
	      size_t i = status->to_compare.n - status->to_compare.start;
	      memcpy (&(status->to_compare.as[i]), &last_a,
		      sizeof (struct bplus_key));
	      memcpy (&(status->to_compare.bs[i]), &last_b,
		      sizeof (struct bplus_key));
	    }
	  status->to_compare.n++;
	}
    }
}

static void
explorer_data (struct bplus_explorer *explorer, size_t row, size_t start,
	       size_t length, const uint32_t * data)
{
  int first_done, last_done, first_found, last_found;
  size_t first_index, last_index;
  struct bplus_key first_a, first_b, last_a, last_b;
  if (explorer->use_shared_analyzer)
    {
      analyzer_match (&(explorer->shared_analyzer), BPLUS_ANALYZER_FIRST,
		      &first_done, &first_found, &first_index, &first_a,
		      &first_b);
      analyzer_match (&(explorer->shared_analyzer), BPLUS_ANALYZER_LAST,
		      &last_done, &last_found, &last_index, &last_a, &last_b);
    }
  else
    {
      analyzer_match (&(explorer->analyze_first), BPLUS_ANALYZER_FIRST,
		      &first_done, &first_found, &first_index, &first_a,
		      &first_b);
      analyzer_match (&(explorer->analyze_last), BPLUS_ANALYZER_LAST,
		      &last_done, &last_found, &last_index, &last_a, &last_b);
    }
  if (first_done && last_done)
    {
      /* Both fetchers have been set up. This is safe. */
      fetcher_data (explorer->fetch_first_child, row, start, length, data);
      fetcher_data (explorer->fetch_last_child, row, start, length, data);
    }
}

static void
explorer_compared (struct bplus_explorer *explorer, const struct bplus_key *a,
		   const struct bplus_key *b, int result)
{
  if (explorer->use_shared_analyzer)
    {
      analyzer_result (&(explorer->shared_analyzer), a, b, result);
    }
  else
    {
      analyzer_result (&(explorer->analyze_first), a, b, result);
      analyzer_result (&(explorer->analyze_last), a, b, result);
    }
  /* Check if the analysis is done */
  int first_done, last_done, first_found, last_found;
  size_t first_index, last_index;
  struct bplus_key first_a, first_b, last_a, last_b;
  if (explorer->use_shared_analyzer)
    {
      analyzer_match (&(explorer->shared_analyzer), BPLUS_ANALYZER_FIRST,
		      &first_done, &first_found, &first_index, &first_a,
		      &first_b);
      analyzer_match (&(explorer->shared_analyzer), BPLUS_ANALYZER_LAST,
		      &last_done, &last_found, &last_index, &last_a, &last_b);
    }
  else
    {
      analyzer_match (&(explorer->analyze_first), BPLUS_ANALYZER_FIRST,
		      &first_done, &first_found, &first_index, &first_a,
		      &first_b);
      analyzer_match (&(explorer->analyze_last), BPLUS_ANALYZER_LAST,
		      &last_done, &last_found, &last_index, &last_a, &last_b);
    }
  if (first_done && last_done)
    {
      /* Set up the fetchers. */
      fetcher_setup (explorer->fetch_first_child,
		     explorer->children_first[first_index]);
      fetcher_setup (explorer->fetch_last_child,
		     explorer->children_last[last_index]);
    }
}

#endif /* not H_BPLUS_EXPLORER_INCLUDED */
