#ifndef H_BPLUS_PARENT_FETCHER_INCLUDED
# define H_BPLUS_PARENT_FETCHER_INCLUDED

# include <bplus.h>
# include "bplus_fetcher.h"

# include <stdlib.h>
# include <string.h>
# include <assert.h>

# define DEALLOC_PARENT_FETCHER \
  ATTRIBUTE_DEALLOC (parent_fetcher_free, 1)

  /* The parent fetcher not only fetches the parent of a given node,
     it also finds where the child is in the parent. More
     specifically, it returns the number of children that are strictly
     before the given child in the parent. It can only request at most
     1 fetch operation. */
struct bplus_parent_fetcher;

static void parent_fetcher_free (struct bplus_parent_fetcher *fetcher);

DEALLOC_PARENT_FETCHER
  static struct bplus_parent_fetcher *parent_fetcher_alloc (struct
							    bplus_tree *tree);

static void parent_fetcher_setup (struct bplus_parent_fetcher *fetcher,
				  uint32_t child_id,
				  const struct bplus_node *child);

  /* If the return value is NULL, then you have to fetch 1 single
     row. Otherwise, you can use the output parameters parent_id and
     parent_position. */
static const struct bplus_node *parent_fetcher_status (const struct
						       bplus_parent_fetcher
						       *fetcher,
						       uint32_t * parent_id,
						       size_t
						       *parent_position,
						       size_t *fetch_row,
						       size_t *fetch_start,
						       size_t *fetch_length);

static void parent_fetcher_data (struct bplus_parent_fetcher *fetcher,
				 size_t row, size_t start, size_t length,
				 const uint32_t * data);

struct bplus_parent_fetcher
{
  uint32_t child_id;
  uint32_t parent_id;
  struct bplus_fetcher *fetcher;
  const struct bplus_node *parent;
  size_t position;
};

static struct bplus_parent_fetcher *
parent_fetcher_alloc (struct bplus_tree *tree)
{
  struct bplus_parent_fetcher *ret =
    malloc (sizeof (struct bplus_parent_fetcher));
  if (ret == NULL)
    {
      return NULL;
    }
  ret->fetcher = fetcher_alloc (tree);
  if (ret->fetcher == NULL)
    {
      fetcher_free (ret->fetcher);
      free (ret);
      return NULL;
    }
  return ret;
}

static void
parent_fetcher_free (struct bplus_parent_fetcher *parent_fetcher)
{
  if (parent_fetcher != NULL)
    {
      fetcher_free (parent_fetcher->fetcher);
    }
  free (parent_fetcher);
}

static void
parent_fetcher_advance (struct bplus_parent_fetcher *parent_fetcher)
{
  if (parent_fetcher->parent == NULL)
    {
      const struct bplus_node *parent =
	fetcher_status (parent_fetcher->fetcher, NULL, NULL, NULL);
      if (parent != NULL)
	{
	  if (parent->is_leaf)
	    {
	      /* Inconsistent data. FIXME: handle the error more
	         gracefully? */
	      /* Do not put an assert here. Asserts are a way to check
	         for a function contract or for internal library
	         purposes, and can be disabled. It is not in any of
	         the function contracts here that the data for the
	         parent must not indicate a leaf, because it’s hard to
	         check without this library. */
	      abort ();
	    }
	  parent_fetcher->parent = parent;
	  /* Increase position up to parent->n_entries + 1 (this value
	     is the number of subtrees, it is reached if the child is
	     not found in its parent). */
	  for (parent_fetcher->position = 0;
	       ((parent_fetcher->position <= parent->n_entries)
		&& (parent->values[parent_fetcher->position] !=
		    parent_fetcher->child_id)); (parent_fetcher->position)++);
	}
    }
}

static void
parent_fetcher_setup (struct bplus_parent_fetcher *parent_fetcher,
		      uint32_t child_id, const struct bplus_node *child)
{
  parent_fetcher->child_id = child_id;
  parent_fetcher->parent_id = child->parent_node;
  fetcher_setup (parent_fetcher->fetcher, parent_fetcher->parent_id);
  parent_fetcher->parent = NULL;
  parent_fetcher_advance (parent_fetcher);
}

static void
parent_fetcher_data (struct bplus_parent_fetcher *parent_fetcher, size_t row,
		     size_t start, size_t length, const uint32_t * data)
{
  if (parent_fetcher->parent == NULL)
    {
      fetcher_data (parent_fetcher->fetcher, row, start, length, data);
      parent_fetcher_advance (parent_fetcher);
    }
}

static const struct bplus_node *
parent_fetcher_status (const struct bplus_parent_fetcher *parent_fetcher,
		       uint32_t * parent_id, size_t *position,
		       size_t *row_to_fetch, size_t *start, size_t *length)
{
  if (parent_fetcher->parent == NULL)
    {
      const struct bplus_node *check =
	fetcher_status (parent_fetcher->fetcher, row_to_fetch, start, length);
      assert (check == NULL);
      /* Otherwise, we failed to parent_fetcher_advance(). */
      return NULL;
    }
  else
    {
      *parent_id = parent_fetcher->parent_id;
      *position = parent_fetcher->position;
      return parent_fetcher->parent;
    }
}

#endif /* H_BPLUS_PARENT_FETCHER_INCLUDED */
