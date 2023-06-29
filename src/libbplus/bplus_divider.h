#ifndef H_BPLUS_DIVIDER_INCLUDED
# define H_BPLUS_DIVIDER_INCLUDED

# include <bplus.h>

# include <stdlib.h>
# include <string.h>
# include <assert.h>
# include <stdbool.h>

# define DEALLOC_DIVIDER \
  ATTRIBUTE_DEALLOC (divider_free, 1)

  /* The node divider is responsible for taking a node and an extra
     record to add, allocate a new node if needed, and deal the
     records more equally between both if the new record does not fit
     in. It does not actually emit the store requests yet. */
struct bplus_divider;

static void divider_free (struct bplus_divider *divider);

DEALLOC_DIVIDER static struct bplus_divider *divider_alloc (size_t order);

static void divider_setup (struct bplus_divider *divider, uint32_t node_id,
			   const struct bplus_node *node, uint32_t extra_key,
			   uint32_t extra_value);

  /* new_node is set to NULL, and *new_node_id to -1, if no new node
     is required. */
static void divider_status (const struct bplus_divider *divider, int *done,
			    const struct bplus_node **updated,
			    uint32_t * new_node_id,
			    uint32_t * new_node_key,
			    const struct bplus_node **new_node,
			    int *allocation_to_do);

static int divider_allocated (struct bplus_divider *divider, uint32_t new_id);

/* The divider, once set up, will immediately try to just append the
   extra_key, extra_value to the node. If it could do so,
   allocation_required is set to false, and new_node_id, new_node and
   saved_key are left uninitialized. Otherwise, it immediately sets
   allocation_required to true, and sets new_node_id to -1. */

/* The divider expects a node to be allocated as long as
   allocation_required is true, and new_node_id is -1. Check those 2
   conditions in that order, because if allocation_required is false,
   then new_node_id is uninitialized. */

/* When the allocation node ID is known, the node and new_node are
   modified, and saved_key is set. */

struct bplus_divider
{
  size_t order;
  uint32_t node_id;
  struct bplus_node node;
  uint32_t extra_key;
  uint32_t extra_value;
  bool allocation_required;
  uint32_t new_node_id;
  struct bplus_node new_node;
  uint32_t saved_key;
};

static struct bplus_divider *
divider_alloc (size_t order)
{
  struct bplus_divider *ret = malloc (sizeof (struct bplus_divider));
  if (ret == NULL)
    {
      return NULL;
    }
  ret->order = order;
  ret->node.keys = malloc ((order - 1) * sizeof (uint32_t));
  ret->node.values = malloc (order * sizeof (uint32_t));
  ret->new_node.keys = malloc ((order - 1) * sizeof (uint32_t));
  ret->new_node.values = malloc (order * sizeof (uint32_t));
  if (ret->node.keys == NULL || ret->node.values == NULL
      || ret->new_node.keys == NULL || ret->new_node.values == NULL)
    {
      free (ret->node.keys);
      free (ret->node.values);
      free (ret->new_node.keys);
      free (ret->new_node.values);
      free (ret);
      return NULL;
    }
  return ret;
}

static void
divider_free (struct bplus_divider *divider)
{
  if (divider != NULL)
    {
      free (divider->node.keys);
      free (divider->node.values);
      free (divider->new_node.keys);
      free (divider->new_node.values);
    }
  free (divider);
}

static void
divider_setup (struct bplus_divider *divider, uint32_t node_id,
	       const struct bplus_node *node, uint32_t extra_key,
	       uint32_t extra_value)
{
  divider->node_id = node_id;
  bplus_node_copy (divider->order, &(divider->node), node);
  divider->extra_key = extra_key;
  divider->extra_value = extra_value;
  if (node->n_entries < divider->order - 1)
    {
      /* There is room, no need to allocate. */
      size_t key_position = node->n_entries;
      size_t value_position = node->n_entries + 1;
      if (node->is_leaf)
	{
	  value_position = node->n_entries;
	}
      divider->node.keys[key_position] = extra_key;
      divider->node.values[value_position] = extra_value;
      divider->extra_key = ((uint32_t) (-1));
      divider->allocation_required = false;
      divider->node.n_entries = key_position + 1;
    }
  else
    {
      /* There is not enough room, we need to wait for a new ID. */
      divider->allocation_required = true;
      divider->new_node_id = ((uint32_t) (-1));
    }
}

static int
divider_allocated (struct bplus_divider *divider, uint32_t new_id)
{
  if (divider->allocation_required && divider->extra_key != ((uint32_t) (-1)))
    {
      divider->new_node_id = new_id;
      bplus_node_copy (divider->order, &(divider->new_node),
		       &(divider->node));
      const size_t n_to_add = 1;
      const size_t n_values_total_if_leaf =
	divider->node.n_entries + n_to_add;
      const size_t n_values_total_if_node =
	(divider->node.n_entries + 1) + n_to_add;
      size_t n_values_total = n_values_total_if_node;
      if (divider->node.is_leaf)
	{
	  n_values_total = n_values_total_if_leaf;
	}
      const size_t n_values_given = n_values_total / 2;
      const size_t n_values_kept = n_values_total - n_values_given;
      size_t n_keys_given = n_values_given - 1;
      size_t n_keys_kept = n_values_kept - 1;
      if (divider->node.is_leaf)
	{
	  n_keys_given = n_values_given;
	  n_keys_kept = n_values_kept;
	  /* Fix the leaf chain: */
	  divider->node.values[divider->order - 1] = new_id;
	}
      const size_t first_key_to_give = n_values_kept;
      divider->saved_key = divider->node.keys[first_key_to_give - 1];
      /* Give some records to the new node.  Start with the keys. */
      divider->new_node.n_entries = n_keys_given;
      for (size_t i = 0; i < n_keys_given; i++)
	{
	  uint32_t key_to_give = divider->extra_key;
	  const size_t position_in_parent = first_key_to_give + i;
	  assert (position_in_parent <= divider->order - 1);
	  if (position_in_parent < divider->order - 1)
	    {
	      key_to_give = divider->node.keys[position_in_parent];
	    }
	  divider->new_node.keys[i] = key_to_give;
	}
      /* Now the values. */
      for (size_t i = 0; i < n_values_given; i++)
	{
	  uint32_t value_to_give = divider->extra_value;
	  const size_t position_in_parent = n_values_kept + i;
	  size_t max = divider->order;
	  if (divider->node.is_leaf)
	    {
	      max = divider->order - 1;
	    }
	  assert (position_in_parent <= max);
	  if (position_in_parent < max)
	    {
	      value_to_give = divider->node.values[position_in_parent];
	    }
	  divider->new_node.values[i] = value_to_give;
	}
      if (divider->node.is_leaf)
	{
	  assert (n_keys_given == n_values_given);
	  assert (n_keys_kept == n_values_kept);
	  assert (n_keys_given + n_keys_kept == n_values_total);
	  assert (n_values_given + n_values_kept == n_values_total);
	}
      else
	{
	  assert (n_keys_given + 1 == n_values_given);
	  assert (n_keys_kept + 1 == n_values_kept);
	  const size_t n_keys_saved = 1;
	  assert (n_keys_given + n_keys_kept + n_keys_saved ==
		  divider->node.n_entries + 1);
	  assert (n_values_given + n_values_kept == n_values_total);
	}
      divider->new_node.n_entries = n_keys_given;
      divider->node.n_entries = n_keys_kept;
      return 1;
    }
  return 0;
}

static void
divider_status (const struct bplus_divider *divider, int *done,
		const struct bplus_node **updated, uint32_t * new_node_id,
		uint32_t * new_node_key, const struct bplus_node **new_node,
		int *allocation_to_do)
{
  const bool no_allocation_required = !(divider->allocation_required);
  const bool has_allocated =
    (no_allocation_required ?
     false : (divider->new_node_id != ((uint32_t) (-1))));
  *done = no_allocation_required || has_allocated;
  if (*done)
    {
      *updated = &(divider->node);
      *new_node_id = ((uint32_t) (-1));
      *new_node = NULL;
      if (divider->allocation_required)
	{
	  assert (divider->new_node_id != ((uint32_t) (-1)));
	  *new_node_id = divider->new_node_id;
	  *new_node_key = divider->saved_key;
	  *new_node = &(divider->new_node);
	}
      *allocation_to_do = 0;
    }
  else
    {
      *allocation_to_do = 1;
    }
}

#endif /* H_BPLUS_DIVIDER_INCLUDED */
