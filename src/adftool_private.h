#ifndef H_ADFTOOL_PRIVATE_INCLUDED
#define H_ADFTOOL_PRIVATE_INCLUDED

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <adftool.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <locale.h>

#include "gettext.h"

enum adftool_bplus_key_type
{
  ADFTOOL_BPLUS_KEY_KNOWN,
  ADFTOOL_BPLUS_KEY_UNKNOWN
};

union adftool_bplus_key_arg
{
  uint32_t known;
  void *unknown;
};

struct adftool_bplus_key
{
  enum adftool_bplus_key_type type;
  union adftool_bplus_key_arg arg;
};

enum adftool_bplus_fetch_context_type
{
  ADFTOOL_BPLUS_FETCH_UNSET,
  ADFTOOL_BPLUS_FETCH_LOGICAL,
  ADFTOOL_BPLUS_FETCH_HDF5
};

union adftool_bplus_fetch_context_arg
{
  void *logical;
  hid_t hdf5;
};

struct adftool_bplus_fetch_context
{
  enum adftool_bplus_fetch_context_type type;
  union adftool_bplus_fetch_context_arg arg;
};

struct adftool_bplus_parameters
{
  int (*fetch) (uint32_t, size_t *, size_t, size_t, uint32_t *, void *);
  struct adftool_bplus_fetch_context fetch_context;
  int (*compare) (const struct adftool_bplus_key *,
		  const struct adftool_bplus_key *, int *, void *);
  void *compare_context;
};

struct node
{
  uint32_t id;
  size_t order;
  uint32_t *row;		/* 2 * order + 1 elements allocated. */
};

static inline int
node_init (size_t order, uint32_t id, struct node *node)
{
  node->id = id;
  node->order = order;
  node->row = malloc ((2 * node->order + 1) * sizeof (uint32_t));
  if (node->row == NULL)
    {
      return 1;
    }
  assert (order > 0);
  for (size_t i = 0; i + 1 < order; i++)
    {
      node->row[i] = (uint32_t) (-1);
    }
  for (size_t i = order - 1; i < 2 * order - 1; i++)
    {
      node->row[i] = 0;
    }
  node->row[2 * order - 1] = -1;
  node->row[2 * order] = 0;
  return 0;
}

static inline void
node_clean (struct node *node)
{
  free (node->row);
}

static inline int
node_set_order (struct node *node, size_t order)
{
  if (order != node->order)
    {
      /* Realloc the row */
      uint32_t *reallocated =
	realloc (node->row, (2 * order + 1) * sizeof (uint32_t));
      if (reallocated == NULL)
	{
	  return 1;
	}
      assert (order > 0);
      for (size_t i = 0; i + 1 < order; i++)
	{
	  reallocated[i] = (uint32_t) (-1);
	}
      for (size_t i = order - 1; i < 2 * order - 1; i++)
	{
	  reallocated[i] = 0;
	}
      reallocated[2 * order - 1] = -1;
      reallocated[2 * order] = 0;
      node->order = order;
      node->row = reallocated;
    }
  return 0;
}

static inline int
node_is_leaf (const struct node *node)
{
  uint32_t flags = node->row[2 * node->order];
  uint32_t bit = 1;
  uint32_t flag_is_leaf = bit << 31;
  return (flags & flag_is_leaf) != 0;
}

static uint32_t
node_key (const struct node *node, size_t i)
{
  assert (i + 1 < node->order);

  return node->row[i];
}

static uint32_t
node_value (const struct node *node, size_t i)
{
  assert (i < node->order);
  return node->row[node->order - 1 + i];
}

static uint32_t
node_next_leaf (const struct node *node)
{
  assert (node_is_leaf (node));
  assert (node->order > 0);
  return node_value (node, node->order - 1);
}

static uint32_t
node_parent (const struct node *node)
{
  assert (node->order > 0);
  return node->row[2 * node->order - 1];
}

static inline int
adftool_bplus_parameters_fetch (const struct adftool_bplus_parameters
				*parameters, uint32_t row_id,
				size_t *actual_row_length,
				size_t request_start, size_t request_length,
				uint32_t * response)
{
  switch (parameters->fetch_context.type)
    {
    case ADFTOOL_BPLUS_FETCH_UNSET:
      break;
    case ADFTOOL_BPLUS_FETCH_LOGICAL:
      return parameters->fetch (row_id, actual_row_length, request_start,
				request_length, response,
				parameters->fetch_context.arg.logical);
    case ADFTOOL_BPLUS_FETCH_HDF5:
      return parameters->fetch (row_id, actual_row_length, request_start,
				request_length, response,
				(void
				 *) (&(parameters->fetch_context.arg.hdf5)));
    }
  assert (0);
  return 0;
}

static inline int
node_fetch (const struct adftool_bplus_parameters *parameters,
	    uint32_t id, struct node *node)
{
  size_t actual_row_length;
  node->id = id;
  while (1)
    {
      int fetch_error =
	adftool_bplus_parameters_fetch (parameters, node->id,
					&actual_row_length, 0,
					2 * node->order + 1, node->row);
      if (fetch_error)
	{
	  return 1;
	}
      else if (actual_row_length != 2 * node->order + 1)
	{
	  assert ((actual_row_length % 2) == 1);
	  int grow_error = node_set_order (node, (actual_row_length - 1) / 2);
	  if (grow_error)
	    {
	      return 1;
	    }
	  fetch_error =
	    adftool_bplus_parameters_fetch (parameters, node->id,
					    &actual_row_length, 0,
					    2 * node->order + 1, node->row);
	  if (fetch_error)
	    {
	      return 1;
	    }
	  else if (actual_row_length == 2 * node->order + 1)
	    {
	      break;
	    }
	}
      else
	{
	  break;
	}
    }
  return 0;
}

static inline int
adftool_bplus_parameters_compare (const struct adftool_bplus_parameters
				  *parameters,
				  const struct adftool_bplus_key *key_a,
				  const struct adftool_bplus_key *key_b,
				  int *result)
{
  assert (parameters->compare != NULL);
  return parameters->compare (key_a, key_b, result,
			      parameters->compare_context);
}

void _adftool_ensure_init (void);

static inline void
ensure_init (void)
{
  static int is_initialized = 0;
  if (!is_initialized)
    {
      _adftool_ensure_init ();
      is_initialized = 1;
    }
}

#endif /* not H_ADFTOOL_PRIVATE_INCLUDED */
