#ifndef H_BPLUS_RANGE_INCLUDED
# define H_BPLUS_RANGE_INCLUDED

# include <bplus.h>

# include <stdlib.h>
# include <string.h>

  /* A range is a list of leaves, with a start and stop key
     position. You can iterate over the range. */
struct bplus_range;

static inline struct bplus_range *range_alloc (struct bplus_tree *tree);

static inline void range_free (struct bplus_range *range);

  /* May fail if the head or tail aren’t leaves. In this case, return
     non-zero. Otherwise, return 0. */
static inline
  int range_setup (struct bplus_range *range, uint32_t head_id,
		   const struct bplus_node *head, size_t first_key,
		   uint32_t tail_id, const struct bplus_node *tail,
		   size_t stop_key);

  /* Return the number of records that are available in the head, and
     fill keys and values up to that (discard the start first). Do not
     touch the output arrays at or after the max index. Call
     range_next to examine the next leaf, don’t stop there! If
     there are records to process in the next leaf, set *has_next to 1
     (otherwise, set it to 0). If tree rows must be loaded before you
     can consider the next leaf, then set *n_load_requests to the
     number of requests to make, and fill *load_request_row,
     *load_request_start and *load_request_length with the requests to
     load (up to max). The first start_load_requests are discarded. */
static inline
  size_t range_get (const struct bplus_range *range, size_t start,
		    size_t max, struct bplus_key *keys,
		    uint32_t * values, int *has_next,
		    size_t *n_load_requests,
		    size_t start_load_requests,
		    size_t max_load_requests,
		    size_t *load_request_row,
		    size_t *load_request_start, size_t *load_request_length);

  /* Prepare the next leaf. After this function call, the head is not
     modified yet. */
static inline
  void range_data (struct bplus_range *range, size_t row,
		   size_t start, size_t length, const uint32_t * data);

  /* Update the head to point to the next leaf. If the data for the
     next leaf is still unknown, or if there are no next leaves at
     all, return non-zero. Otherwise, switch and return
     0. range_get tells you whether you should do this or not,
     see the output parameter has_next. */
static inline int range_next (struct bplus_range *range);

# include "bplus_fetcher.h"

struct bplus_range
{
  size_t order;
  uint32_t head_id;
  struct bplus_node head;
  size_t head_start;

  uint32_t tail_id;
  struct bplus_node tail;
  size_t tail_stop;

  struct bplus_fetcher *fetcher_next;
};

static inline struct bplus_range *
range_alloc (struct bplus_tree *tree)
{
  struct bplus_range *range = malloc (sizeof (struct bplus_range));
  if (range != NULL)
    {
      range->order = tree_order (tree);
      uint32_t *head_keys = malloc ((range->order - 1) * sizeof (uint32_t));
      uint32_t *head_values = malloc (range->order * sizeof (uint32_t));
      uint32_t *tail_keys = malloc ((range->order - 1) * sizeof (uint32_t));
      uint32_t *tail_values = malloc (range->order * sizeof (uint32_t));
      range->fetcher_next = fetcher_alloc (tree);
      if (head_keys == NULL || head_values == NULL || tail_keys == NULL
	  || tail_values == NULL || range->fetcher_next == NULL)
	{
	  free (head_keys);
	  free (head_values);
	  free (tail_keys);
	  free (tail_values);
	  fetcher_free (range->fetcher_next);
	  free (range);
	  range = NULL;
	}
      else
	{
	  node_setup (&(range->head), range->order, head_keys, head_values);
	  node_setup (&(range->tail), range->order, tail_keys, tail_values);
	}
    }
  return range;
}

static inline void
range_free (struct bplus_range *range)
{
  if (range != NULL)
    {
      free (range->head.keys);
      free (range->head.values);
      free (range->tail.keys);
      free (range->tail.values);
      fetcher_free (range->fetcher_next);
    }
  free (range);
}

static inline int
range_setup (struct bplus_range *range, uint32_t head_id,
	     const struct bplus_node *head, size_t first_key,
	     uint32_t tail_id, const struct bplus_node *tail, size_t stop_key)
{
  if ((!node_is_leaf (head)) || (!node_is_leaf (tail)))
    {
      return 1;
    }
  range->head_id = head_id;
  node_copy (range->order, &(range->head), head);
  range->head_start = first_key;
  range->tail_id = tail_id;
  node_copy (range->order, &(range->tail), tail);
  range->tail_stop = stop_key;
  uint32_t next = head->values[range->order - 1];
  if (next != 0 && next != tail_id)
    {
      fetcher_setup (range->fetcher_next, next);
    }
  return 0;
}

static inline size_t
range_get (const struct bplus_range *range, size_t start, size_t max,
	   struct bplus_key *keys, uint32_t * values, int *has_next,
	   size_t *n_load_requests, size_t start_load_requests,
	   size_t max_load_requests, size_t *load_request_row,
	   size_t *load_request_start, size_t *load_request_length)
{
  size_t last_entry = range->head.n_entries;
  if (range->head_id == range->tail_id && range->tail_stop < last_entry)
    {
      last_entry = range->tail_stop;
    }
  assert (last_entry >= range->head_start);
  for (size_t i = 0; i < last_entry - range->head_start; i++)
    {
      if (i >= start && i - start < max)
	{
	  keys[i - start].type = BPLUS_KEY_KNOWN;
	  keys[i - start].arg.known = range->head.keys[i + range->head_start];
	  values[i - start] = range->head.values[i + range->head_start];
	}
    }
  uint32_t next = range->head.values[range->order - 1];
  *has_next = ((range->head_id != range->tail_id) && (next != 0));
  size_t next_row, next_start, next_length;
  if (*has_next && next != range->tail_id)
    {
      const struct bplus_node *next_known =
	fetcher_status (range->fetcher_next, &next_row, &next_start,
			&next_length);
      if (next_known == NULL)
	{
	  *n_load_requests = 1;
	  const size_t i = 0;
	  if (i >= start_load_requests
	      && i - start_load_requests < max_load_requests)
	    {
	      load_request_row[i - start_load_requests] = next_row;
	      load_request_start[i - start_load_requests] = next_start;
	      load_request_length[i - start_load_requests] = next_length;
	    }
	}
      else
	{
	  *n_load_requests = 0;
	}
    }
  else
    {
      /* We are on the last leaf. */
      *n_load_requests = 0;
    }
  return last_entry - range->head_start;
}

static inline void
range_data (struct bplus_range *range, size_t row, size_t start,
	    size_t length, const uint32_t * data)
{
  uint32_t next = range->head.values[range->order - 1];
  if ((next != range->tail_id) && (next != 0))
    {
      fetcher_data (range->fetcher_next, row, start, length, data);
    }
  else
    {
      /* The fetcher may have not been set in the setup function, if
         the head and tail nodes are identical! */
    }
}

static inline int
range_next (struct bplus_range *range)
{
  uint32_t next_id = range->head.values[range->order - 1];
  if (next_id != 0 && range->head_id != range->tail_id
      && next_id != range->tail_id)
    {
      size_t row_to_fetch, start, length;
      const struct bplus_node *next =
	fetcher_status (range->fetcher_next, &row_to_fetch, &start, &length);
      if (next == NULL)
	{
	  /* The next node has not been fetched yet. */
	  return 1;
	}
      else
	{
	  range->head_id = next_id;
	  range->head_start = 0;
	  node_copy (range->order, &(range->head), next);
	  uint32_t next_next = next->values[range->order - 1];
	  if (next_next != 0 && next_next != range->tail_id)
	    {
	      fetcher_setup (range->fetcher_next, next_next);
	    }
	}
    }
  else if (next_id == range->tail_id)
    {
      range->head_id = range->tail_id;
      range->head_start = 0;
      node_copy (range->order, &(range->head), &(range->tail));
    }
  else
    {
      /* There are no next node. */
      return 1;
    }
  return 0;
}

static inline const struct bplus_node *
_range_extremity (const struct bplus_range *range, int last,
		  uint32_t * id, size_t *key_id)
{
  if (last)
    {
      *id = range->tail_id;
      *key_id = range->tail_stop;
      return &(range->tail);
    }
  else
    {
      *id = range->head_id;
      *key_id = range->head_start;
      return &(range->head);
    }
}

#endif /* not H_BPLUS_RANGE_INCLUDED */
