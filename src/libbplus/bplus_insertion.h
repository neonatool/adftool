#ifndef H_BPLUS_INSERTION_INCLUDED
# define H_BPLUS_INSERTION_INCLUDED

# include <bplus.h>

# include <stdlib.h>
# include <string.h>

struct bplus_insertion;

static inline
  struct bplus_insertion *insertion_alloc (struct bplus_tree *tree);

static inline void insertion_free (struct bplus_insertion *insertion);

  /* Start the insertion problem: insert record -> record in the
     tree. If back, insert it as the last position of the
     range. Otherwise, insert it in the first position of the
     range. If the range is empty, both positions refer to the same
     thing. Once the insertion is started, reading through the where
     range is forbidden. */
static inline
  void insertion_setup (struct bplus_insertion *insertion,
			uint32_t record,
			const struct bplus_range *where, int back);

static inline
  void insertion_status (const struct bplus_insertion *insertion, int *done,
			 /* The fetches you have to do */
			 size_t *n_fetches_to_do,
			 size_t start_fetch_to_do,
			 size_t max_fetches_to_do,
			 size_t *fetch_rows,
			 size_t *fetch_starts, size_t *fetch_lengths,
			 /* You also have to allocate nodes */
			 size_t *n_allocations_to_do,
			 /* You also have to update the tree storage */
			 size_t *n_stores_to_do,
			 size_t start_store_to_do,
			 size_t max_stores_to_do,
			 size_t *store_rows,
			 size_t *store_starts,
			 size_t *store_lengths, const uint32_t ** stores);

static inline
  void insertion_data (struct bplus_insertion *insertion, size_t row,
		       size_t start, size_t length, const uint32_t * data);

static inline
  int insertion_allocated (struct bplus_insertion *insertion,
			   uint32_t new_id);

static inline
  void insertion_updated (struct bplus_insertion *insertion, size_t row);

# include "bplus_reparentor.h"
# include "bplus_growth.h"
# include "bplus_parent_fetcher.h"
# include "bplus_divider.h"
# include "bplus_range.h"
# include "bplus_tree.h"
# include "bplus_node.h"

static inline
  const struct bplus_node *_range_extremity (const struct bplus_range
					     *range, int last,
					     uint32_t * id, size_t *key_id);

struct bplus_insertion
{
  size_t order;
  struct bplus_tree *tree;

  uint32_t target_id;
  struct bplus_node target;

  /* Remember that the node divider does not emit store requests, so
     that we can store the updates in parallel with the
     reparentor. However, do not start the new node store, the
     reparentor and the parent fetcher until the growth is done. The
     growth may be unnecessary, in which case it is consider done
     immediately, so in effect everything starts at the same time. */
  struct bplus_divider *divider;
  bool divider_done;
  const struct bplus_node *divider_old;
  uint32_t divider_new_id;
  /* If divider_new is a division of the root, its parent will be
     incorrectly -1 instead of 0. The divider_new_store array will be
     fixed before it is proposed for update, so this is not really a
     problem. */
  const struct bplus_node *divider_new;
  uint32_t divider_pivot_key;
  uint32_t *divider_store;
  uint32_t *divider_new_store;
  bool divider_updated;
  bool divider_new_updated;	/* It is immediately set to true if the
				   divider did not allocate any new
				   node. */

  struct bplus_growth *growth;
  bool growth_started;
  bool growth_done;

  struct bplus_reparentor *reparentor;
  bool reparenting_started;	/* Both are immediately set to true */
  bool reparenting_done;	/* if the divider did not allocate. */

  struct bplus_parent_fetcher *parent_fetcher;
  bool parent_fetcher_started;	/* idem */
  uint32_t parent_id;		/* Don’t read this if divider_new == NULL. */
  const struct bplus_node *parent;
  size_t parent_position;
  bool parent_fetcher_done;
};

static inline struct bplus_insertion *
insertion_alloc (struct bplus_tree *tree)
{
  struct bplus_insertion *ret = malloc (sizeof (struct bplus_insertion));
  if (ret == NULL)
    {
      return NULL;
    }
  ret->order = tree_order (tree);
  ret->tree = tree;
  ret->target.keys = malloc ((ret->order - 1) * sizeof (uint32_t));
  ret->target.values = malloc (ret->order * sizeof (uint32_t));
  ret->divider = divider_alloc (ret->order);
  ret->divider_store = malloc ((2 * ret->order + 1) * sizeof (uint32_t));
  ret->divider_new_store = malloc ((2 * ret->order + 1) * sizeof (uint32_t));
  ret->growth = growth_alloc (tree);
  ret->reparentor = reparentor_alloc (tree);
  ret->parent_fetcher = parent_fetcher_alloc (tree);
  if (ret->target.keys == NULL || ret->target.values == NULL
      || ret->divider == NULL || ret->divider_store == NULL
      || ret->divider_new_store == NULL || ret->growth == NULL
      || ret->reparentor == NULL || ret->parent_fetcher == NULL)
    {
      free (ret->target.keys);
      free (ret->target.values);
      divider_free (ret->divider);
      free (ret->divider_store);
      free (ret->divider_new_store);
      growth_free (ret->growth);
      reparentor_free (ret->reparentor);
      parent_fetcher_free (ret->parent_fetcher);
      free (ret);
      return NULL;
    }
  return ret;
}

static inline void
insertion_free (struct bplus_insertion *insertion)
{
  if (insertion != NULL)
    {
      free (insertion->target.keys);
      free (insertion->target.values);
      divider_free (insertion->divider);
      free (insertion->divider_store);
      free (insertion->divider_new_store);
      growth_free (insertion->growth);
      reparentor_free (insertion->reparentor);
      parent_fetcher_free (insertion->parent_fetcher);
    }
  free (insertion);
}

static void insertion_advance (struct bplus_insertion *insertion);

static inline void
insertion_setup_rec (struct bplus_insertion *insertion, uint32_t key,
		     uint32_t value, uint32_t target_id,
		     const struct bplus_node *target, size_t target_key)
{
  insertion->target_id = target_id;
  node_copy (insertion->order, &(insertion->target), target);
  while (target_key < target->n_entries)
    {
      size_t target_value = target_key + 1;
      if (target->is_leaf)
	{
	  target_value = target_key;
	}
      const uint32_t hold_key = insertion->target.keys[target_key];
      const uint32_t hold_value = insertion->target.values[target_value];
      insertion->target.keys[target_key] = key;
      insertion->target.values[target_value] = value;
      key = hold_key;
      value = hold_value;
      target_key++;
    }
  divider_setup (insertion->divider, target_id, &(insertion->target), key,
		 value);
  insertion->divider_done = false;
  insertion->divider_updated = false;
  insertion->divider_new_updated = false;
  insertion->growth_started = false;
  insertion->growth_done = false;
  insertion->reparenting_started = false;
  insertion->reparenting_done = false;
  insertion->parent_fetcher_started = false;
  insertion->parent_fetcher_done = false;
  insertion_advance (insertion);
}

static inline void
insertion_growth_done (struct bplus_insertion *insertion,
		       uint32_t new_node_0_id)
{
  assert (!(insertion->growth_done));
  insertion->growth_done = true;
  const struct bplus_node *new_node = insertion->divider_new;
  if (new_node_0_id != 0)
    {
      assert (insertion->target_id == 0);
      insertion->target_id = new_node_0_id;
      assert (insertion->target.parent_node == ((uint32_t) (-1)));
      insertion->target.parent_node = 0;
    }
  if (new_node != NULL)
    {
      if (insertion->divider_new->parent_node == ((uint32_t) (-1)))
	{
	  /* This is incorrect after the growth (it should be changed
	     to 0). We waited for the growth to be done before
	     updating the new node. We should also fix
	     insertion->divider_new->parent_node, but since the
	     reparentor does not update its argument, we won’t
	     override the result of the growth. */
	  insertion->divider_new_store[2 * insertion->order - 1] = 0;
	  assert (new_node_0_id != 0);
	}
      else
	{
	  assert (new_node_0_id == 0);
	}
      reparentor_setup (insertion->reparentor, insertion->divider_new_id,
			new_node);
      parent_fetcher_setup (insertion->parent_fetcher, insertion->target_id,
			    &(insertion->target));
      insertion->reparenting_started = true;
      insertion->reparenting_done = false;
      insertion->parent_fetcher_started = true;
      insertion->parent_fetcher_done = false;
      insertion_advance (insertion);
      return;
    }
  else
    {
      insertion->reparenting_started = true;
      insertion->reparenting_done = true;
      insertion->parent_fetcher_started = true;
      insertion->parent_fetcher_done = true;
      insertion_advance (insertion);
      return;
    }
}

static void
insertion_advance (struct bplus_insertion *insertion)
{
  if (!insertion->divider_done)
    {
      int done;
      const struct bplus_node *updated;
      uint32_t new_id;
      uint32_t pivot;
      const struct bplus_node *new_node;
      int requires_allocation;
      divider_status (insertion->divider, &done, &updated, &new_id, &pivot,
		      &new_node, &requires_allocation);
      if (done)
	{
	  insertion->divider_done = true;
	  insertion->divider_old = updated;
	  insertion->divider_new_id = new_id;
	  insertion->divider_new = new_node;
	  if (new_node != NULL)
	    {
	      insertion->divider_pivot_key = pivot;
	    }
	  node_copy (insertion->order, &(insertion->target), updated);
	  /* Now, prepare the updates: */
	  /* Keys: */
	  for (size_t i = 0; i < insertion->order - 1; i++)
	    {
	      insertion->divider_store[i] = ((uint32_t) (-1));
	      insertion->divider_new_store[i] = ((uint32_t) (-1));
	      if (i < insertion->divider_old->n_entries)
		{
		  insertion->divider_store[i] =
		    insertion->divider_old->keys[i];
		}
	      if (insertion->divider_new != NULL
		  && i < insertion->divider_new->n_entries)
		{
		  insertion->divider_new_store[i] =
		    insertion->divider_new->keys[i];
		}
	    }
	  /* Values: */
	  for (size_t i = 0; i < insertion->order; i++)
	    {
	      insertion->divider_store[insertion->order - 1 + i] = 0;
	      insertion->divider_new_store[insertion->order - 1 + i] = 0;
	      const bool i_is_last = (i == insertion->order - 1);
	      const bool i_is_last_old_entry =
		(i == insertion->divider_old->n_entries);
	      const bool i_is_last_new_entry =
		(insertion->divider_new != NULL
		 && i == insertion->divider_new->n_entries);
	      const bool target_is_leaf = insertion->target.is_leaf;
	      const bool i_is_old_valid =
		(i < insertion->divider_old->n_entries
		 || (target_is_leaf ? i_is_last : i_is_last_old_entry));
	      const bool i_is_new_valid =
		(insertion->divider_new != NULL
		 && (i < insertion->divider_new->n_entries
		     || (target_is_leaf ? i_is_last : i_is_last_new_entry)));
	      if (i_is_old_valid)
		{
		  insertion->divider_store[insertion->order - 1 + i] =
		    insertion->divider_old->values[i];
		}
	      if (i_is_new_valid)
		{
		  insertion->divider_new_store[insertion->order - 1 + i] =
		    insertion->divider_new->values[i];
		}
	    }
	  /* Parents: */
	  insertion->divider_store[2 * insertion->order - 1] =
	    insertion->divider_old->parent_node;
	  if (insertion->divider_new != NULL)
	    {
	      insertion->divider_new_store[2 * insertion->order - 1] =
		insertion->divider_new->parent_node;
	    }
	  /* Flags: */
	  static const uint32_t one = 1;
	  static const uint32_t mask = one << 31;
	  insertion->divider_store[2 * insertion->order] =
	    (insertion->divider_old->is_leaf ? mask : 0);
	  if (insertion->divider_new != NULL)
	    {
	      insertion->divider_new_store[2 * insertion->order] =
		(insertion->divider_new->is_leaf ? mask : 0);
	    }
	  insertion->divider_updated = false;
	  insertion->divider_new_updated = false;
	  if (insertion->divider_new == NULL)
	    {
	      insertion->divider_new_updated = true;
	    }
	  /* Now set up the other steps: fix the parents of the new
	     node, and prepare the recursive insertion. */
	  if (insertion->divider_old->parent_node == ((uint32_t) (-1)))
	    {
	      assert (insertion->target_id == 0);
	    }
	  else
	    {
	      assert (insertion->target_id != 0);
	    }
	  assert ((insertion->divider_old->parent_node == ((uint32_t) (-1)))
		  == (insertion->target_id == 0));
	  if (insertion->divider_new != NULL
	      && insertion->divider_old->parent_node == ((uint32_t) (-1)))
	    {
	      /* We have to grow. The rest of the code makes sure that
	         the growth is not set up until after the update has
	         been saved, and that the reparentor / parent fetcher
	         are not set up until after the growth is done. */
	      insertion->growth_started = false;
	      insertion->growth_done = false;
	      insertion_advance (insertion);
	    }
	  else if (insertion->divider_new != NULL)
	    {
	      /* No need to grow at all. */
	      insertion->growth_started = true;
	      insertion_growth_done (insertion, 0);
	    }
	  return;
	}
    }
  if (insertion->divider_done && insertion->divider_updated
      && insertion->divider_new != NULL
      && insertion->divider_old->parent_node == ((uint32_t) (-1))
      && !(insertion->growth_started))
    {
      insertion->growth_started = true;
      growth_setup (insertion->growth);
      insertion_advance (insertion);
      return;
    }
  if (insertion->divider_done && insertion->divider_updated
      && insertion->growth_started && !(insertion->growth_done))
    {
      assert (insertion->target.parent_node == ((uint32_t) (-1)));
      assert (insertion->target_id == 0);
      int done;
      size_t growth_fetches, growth_allocations, growth_stores;
      uint32_t new_id;
      growth_status (insertion->growth, &done, &new_id, &growth_fetches, 0, 0,
		     NULL, NULL, NULL, &growth_allocations, &growth_stores, 0,
		     0, NULL, NULL, NULL, NULL);
      if (done)
	{
	  insertion_growth_done (insertion, new_id);
	  insertion_advance (insertion);
	  return;
	}
    }
  if (insertion->divider_done && insertion->reparenting_started
      && !(insertion->reparenting_done))
    {
      int reparenting_done;
      size_t n_fetches, n_stores;
      reparentor_status (insertion->reparentor, &reparenting_done, &n_fetches,
			 0, 0, NULL, NULL, NULL, &n_stores, 0, 0, NULL, NULL,
			 NULL, NULL);
      if (reparenting_done)
	{
	  insertion->reparenting_done = true;
	  insertion_advance (insertion);
	  return;
	}
    }
  if (insertion->divider_done && insertion->parent_fetcher_started
      && !(insertion->parent_fetcher_done))
    {
      uint32_t parent_id;
      const struct bplus_node *parent;
      size_t parent_position;
      parent =
	parent_fetcher_status (insertion->parent_fetcher, &parent_id,
			       &parent_position, NULL, NULL, NULL);
      if (parent != NULL)
	{
	  insertion->parent_id = parent_id;
	  insertion->parent = parent;
	  insertion->parent_position = parent_position;
	  insertion->parent_fetcher_done = true;
	  insertion_advance (insertion);
	  return;
	}
    }
  if (insertion->divider_updated && insertion->divider_new_updated
      && insertion->reparenting_done && insertion->parent_fetcher_done
      && insertion->divider_new != NULL)
    {
      /* A recursive call is required. */
      insertion_setup_rec (insertion, insertion->divider_pivot_key,
			   insertion->divider_new_id, insertion->parent_id,
			   insertion->parent, insertion->parent_position);
    }
}

static inline void
insertion_setup (struct bplus_insertion *insertion, uint32_t record,
		 const struct bplus_range *where, int back)
{
  uint32_t target_id;
  size_t target_key;
  const struct bplus_node *target =
    _range_extremity (where, back, &target_id, &target_key);
  insertion_setup_rec (insertion, record, record, target_id, target,
		       target_key);
}

static inline void
insertion_status (const struct bplus_insertion *insertion,
		  int *done,
		  size_t *n_fetches_to_do,
		  size_t start_fetch_to_do,
		  size_t max_fetches_to_do,
		  size_t *fetch_rows,
		  size_t *fetch_starts, size_t *fetch_lengths,
		  /* You also have to allocate nodes */
		  size_t *n_allocations_to_do,
		  /* You also have to update the tree storage */
		  size_t *n_stores_to_do,
		  size_t start_store_to_do,
		  size_t max_stores_to_do,
		  size_t *store_rows,
		  size_t *store_starts,
		  size_t *store_lengths, const uint32_t ** stores)
{
  *done = 0;
  *n_fetches_to_do = 0;
  *n_allocations_to_do = 0;
  *n_stores_to_do = 0;
  if (!insertion->divider_done)
    {
      /* The divider wants to allocate. */
      int sub_done;
      const struct bplus_node *updated;
      uint32_t new_node_id, new_node_key;
      const struct bplus_node *new_node;
      int allocation_to_do;
      divider_status (insertion->divider, &sub_done, &updated, &new_node_id,
		      &new_node_key, &new_node, &allocation_to_do);
      assert (!sub_done);
      assert (allocation_to_do);
      *n_allocations_to_do += 1;
    }
  if (insertion->divider_done && !(insertion->divider_updated))
    {
      /* Push the request to update the main node. */
      const size_t i = *n_stores_to_do;
      if (i >= start_store_to_do && i - start_store_to_do < max_stores_to_do)
	{
	  const size_t output_index = i - start_store_to_do;
	  store_rows[output_index] = insertion->target_id;
	  store_starts[output_index] = 0;
	  store_lengths[output_index] = 2 * insertion->order + 1;
	  stores[output_index] = insertion->divider_store;
	}
      *n_stores_to_do = i + 1;
    }
  if (insertion->divider_done && insertion->growth_done
      && !(insertion->divider_new_updated))
    {
      /* Push the request to update the allocated node. */
      assert (insertion->divider_new != NULL);
      const size_t i = *n_stores_to_do;
      if (i >= start_store_to_do && i - start_store_to_do < max_stores_to_do)
	{
	  const size_t output_index = i - start_store_to_do;
	  store_rows[output_index] = insertion->divider_new_id;
	  store_starts[output_index] = 0;
	  store_lengths[output_index] = 2 * insertion->order + 1;
	  stores[output_index] = insertion->divider_new_store;
	}
      *n_stores_to_do = i + 1;
    }
  if (insertion->divider_done && insertion->growth_started
      && insertion->growth_done && !(insertion->reparenting_done))
    {
      /* We want the reparentor to add its fetch and store requests to
         our array arguments, but we must be careful about the indices
         so as to not erase what we have pushed so far. */
      size_t sub_n_fetches, sub_n_stores;
      int sub_done;
      size_t sub_start_fetch = 0;
      /* sub_start_fetch is max (start_fetch_to_do - *n_fetches, 0) */
      size_t n_valid_taken = 0;
      /* n_valid_taken is the number of slots that have already been
         filled earlier: */
      /* max (*n_fetches - start_fetch_to_do, 0) */
      size_t sub_max_fetches = 0;
      /* sub_max_fetches is max (max_fetches_to_do - n_valid_taken,
         0) */
      if (start_fetch_to_do > *n_fetches_to_do)
	{
	  sub_start_fetch = start_fetch_to_do - *n_fetches_to_do;
	}
      else
	{
	  n_valid_taken = *n_fetches_to_do - start_fetch_to_do;
	}
      if (max_fetches_to_do > n_valid_taken)
	{
	  sub_max_fetches = max_fetches_to_do - n_valid_taken;
	}
      size_t *sub_fetch_rows = NULL;
      size_t *sub_fetch_starts = NULL;
      size_t *sub_fetch_lengths = NULL;
      if (sub_max_fetches != 0)
	{
	  sub_fetch_rows = fetch_rows + *n_fetches_to_do;
	  sub_fetch_starts = fetch_starts + *n_fetches_to_do;
	  sub_fetch_lengths = fetch_lengths + *n_fetches_to_do;
	}
      /* Do the same for the stores. */
      size_t sub_start_store = 0;
      /* sub_start_store is max (start_store_to_do - *n_stores, 0) */
      n_valid_taken = 0;
      /* n_valid_taken is the number of slots that have already been
         filled earlier: */
      /* max (*n_stores - start_store_to_do, 0) */
      size_t sub_max_stores = 0;
      /* sub_max_stores is max (max_stores_to_do - n_valid_taken,
         0) */
      if (start_store_to_do > *n_stores_to_do)
	{
	  sub_start_store = start_store_to_do - *n_stores_to_do;
	}
      else
	{
	  n_valid_taken = *n_stores_to_do - start_store_to_do;
	}
      if (max_stores_to_do > n_valid_taken)
	{
	  sub_max_stores = max_stores_to_do - n_valid_taken;
	}
      size_t *sub_store_rows = NULL;
      size_t *sub_store_starts = NULL;
      size_t *sub_store_lengths = NULL;
      const uint32_t **sub_stores = NULL;
      if (sub_max_stores != 0)
	{
	  sub_store_rows = store_rows + *n_stores_to_do;
	  sub_store_starts = store_starts + *n_stores_to_do;
	  sub_store_lengths = store_lengths + *n_stores_to_do;
	  sub_stores = stores + *n_stores_to_do;
	}
      reparentor_status (insertion->reparentor, &sub_done, &sub_n_fetches,
			 sub_start_fetch, sub_max_fetches, sub_fetch_rows,
			 sub_fetch_starts, sub_fetch_lengths, &sub_n_stores,
			 sub_start_store, sub_max_stores, sub_store_rows,
			 sub_store_starts, sub_store_lengths, sub_stores);
      assert (!sub_done);
      *n_fetches_to_do += sub_n_fetches;
      *n_stores_to_do += sub_n_stores;
    }
  if (insertion->divider_done && insertion->growth_started
      && insertion->growth_done && insertion->parent_fetcher_started
      && !(insertion->parent_fetcher_done))
    {
      uint32_t parent_id;
      size_t parent_position;
      size_t new_row_to_fetch, start_row_to_fetch, row_length_to_fetch;
      const struct bplus_node *parent = NULL;
      parent =
	parent_fetcher_status (insertion->parent_fetcher, &parent_id,
			       &parent_position, &new_row_to_fetch,
			       &start_row_to_fetch, &row_length_to_fetch);
      assert (parent == NULL);
      const size_t i = *n_fetches_to_do;
      if (i >= start_fetch_to_do && i - start_fetch_to_do < max_fetches_to_do)
	{
	  const size_t output_index = i - start_fetch_to_do;
	  fetch_rows[output_index] = new_row_to_fetch;
	  fetch_starts[output_index] = start_row_to_fetch;
	  fetch_lengths[output_index] = row_length_to_fetch;
	}
      *n_fetches_to_do += 1;
    }
  if (insertion->divider_done && insertion->divider_updated
      && insertion->growth_started && !(insertion->growth_done))
    {
      int sub_done;
      uint32_t new_id;
      size_t sub_n_fetches_to_do;
      size_t sub_start_fetch_to_do = 0;
      size_t sub_max_fetches_to_do = 0;
      size_t *sub_fetch_rows = NULL;
      size_t *sub_fetch_starts = NULL;
      size_t *sub_fetch_lengths = NULL;
      size_t sub_n_allocations_to_do;
      size_t sub_n_stores_to_do;
      size_t sub_start_store_to_do = 0;
      size_t sub_max_stores_to_do = 0;
      size_t *sub_store_rows = NULL;
      size_t *sub_store_starts = NULL;
      size_t *sub_store_lengths = NULL;
      const uint32_t **sub_stores = NULL;
      size_t n_valid_fetches_taken = 0;
      size_t n_valid_stores_taken = 0;
      if (start_fetch_to_do > *n_fetches_to_do)
	{
	  sub_start_fetch_to_do = start_fetch_to_do - *n_fetches_to_do;
	}
      else
	{
	  n_valid_fetches_taken = *n_fetches_to_do - start_fetch_to_do;
	}
      if (max_fetches_to_do > n_valid_fetches_taken)
	{
	  sub_max_fetches_to_do = max_fetches_to_do - n_valid_fetches_taken;
	}
      if (start_store_to_do > *n_stores_to_do)
	{
	  sub_start_store_to_do = start_store_to_do - *n_stores_to_do;
	}
      else
	{
	  n_valid_stores_taken = *n_stores_to_do - start_store_to_do;
	}
      if (max_stores_to_do > n_valid_stores_taken)
	{
	  sub_max_stores_to_do = max_stores_to_do - n_valid_stores_taken;
	}
      if (sub_max_fetches_to_do != 0)
	{
	  sub_fetch_rows = fetch_rows + *n_fetches_to_do;
	  sub_fetch_starts = fetch_starts + *n_fetches_to_do;
	  sub_fetch_lengths = fetch_lengths + *n_fetches_to_do;
	}
      if (sub_max_stores_to_do != 0)
	{
	  sub_store_rows = store_rows + *n_stores_to_do;
	  sub_store_starts = store_starts + *n_stores_to_do;
	  sub_store_lengths = store_lengths + *n_stores_to_do;
	  sub_stores = stores + *n_stores_to_do;
	}
      growth_status (insertion->growth, &sub_done, &new_id,
		     &sub_n_fetches_to_do, sub_start_fetch_to_do,
		     sub_max_fetches_to_do, sub_fetch_rows, sub_fetch_starts,
		     sub_fetch_lengths, &sub_n_allocations_to_do,
		     &sub_n_stores_to_do, sub_start_store_to_do,
		     sub_max_stores_to_do, sub_store_rows, sub_store_starts,
		     sub_store_lengths, sub_stores);
      *n_fetches_to_do += sub_n_fetches_to_do;
      *n_allocations_to_do += sub_n_allocations_to_do;
      *n_stores_to_do += sub_n_stores_to_do;
    }
  if (insertion->divider_done && insertion->divider_updated
      && insertion->divider_new == NULL)
    {
      *done = 1;
    }
}

static inline void
insertion_data (struct bplus_insertion *insertion, size_t row,
		size_t start, size_t length, const uint32_t * data)
{
  if (insertion->divider_done && insertion->divider_updated
      && insertion->growth_started && !(insertion->growth_done))
    {
      growth_data (insertion->growth, row, start, length, data);
    }
  if (insertion->divider_done && insertion->reparenting_started
      && !(insertion->reparenting_done))
    {
      reparentor_data (insertion->reparentor, row, start, length, data);
      insertion_advance (insertion);
    }
  if (insertion->divider_done && insertion->parent_fetcher_started
      && !(insertion->parent_fetcher_done))
    {
      parent_fetcher_data (insertion->parent_fetcher, row, start, length,
			   data);
      insertion_advance (insertion);
    }
}

static inline int
insertion_allocated (struct bplus_insertion *insertion, uint32_t id)
{
  if (!(insertion->divider_done))
    {
      int ret = divider_allocated (insertion->divider, id);
      insertion_advance (insertion);
      return ret;
    }
  if (insertion->divider_done && insertion->divider_updated
      && insertion->growth_started && !(insertion->growth_done))
    {
      int ret = growth_allocated (insertion->growth, id);
      insertion_advance (insertion);
      return ret;
    }
  return 0;
}

static inline void
insertion_updated (struct bplus_insertion *insertion, size_t row)
{
  if (insertion->divider_done && !(insertion->divider_updated)
      && row == insertion->target_id)
    {
      insertion->divider_updated = true;
      tree_cache (insertion->tree, row, NULL);
      insertion_advance (insertion);
    }
  if (insertion->divider_done && insertion->divider_updated
      && insertion->growth_started && !(insertion->growth_done))
    {
      growth_updated (insertion->growth, row);
      insertion_advance (insertion);
    }
  if (insertion->divider_done && insertion->growth_done
      && !(insertion->divider_new_updated)
      && row == insertion->divider_new_id)
    {
      insertion->divider_new_updated = true;
      tree_cache (insertion->tree, row, NULL);
      insertion_advance (insertion);
    }
  if (insertion->divider_done && insertion->reparenting_started
      && !(insertion->reparenting_done))
    {
      reparentor_updated (insertion->reparentor, row);
      insertion_advance (insertion);
    }
}

#endif /* not H_BPLUS_INSERTION_INCLUDED */
