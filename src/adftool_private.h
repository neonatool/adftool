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

#ifdef BUILDING_LIBADFTOOL
#define _(String) dgettext (PACKAGE, (String))
#define N_(String) (String)
#else
#define _(String) gettext (PACKAGE)
#define N_(String) (String)
#endif

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

enum context_type
{
  UNSET,
  LOGICAL,
  HDF5
};

struct hdf5_context
{
  hid_t dataset;
  hid_t nextID;
};

union context_arg
{
  void *logical;
  struct hdf5_context hdf5;
};

struct context
{
  enum context_type type;
  union context_arg arg;
};

struct adftool_bplus
{
  int (*fetch) (uint32_t, size_t *, size_t, size_t, uint32_t *, void *);
  struct context fetch_context;
  int (*compare) (const struct adftool_bplus_key *,
		  const struct adftool_bplus_key *, int *, void *);
  void *compare_context;
  void (*allocate) (uint32_t *, void *);
  struct context allocate_context;
  void (*store) (uint32_t, size_t, size_t, const uint32_t *, void *);
  struct context store_context;
};

struct node
{
  uint32_t id;
  size_t order;
  uint32_t *row;		/* 2 * order + 1 elements allocated. */
};

static inline int node_init (size_t order, uint32_t id, struct node *node);
static inline void node_clean (struct node *node);
static inline int node_set_order (struct node *node, size_t order);
static inline int node_is_leaf (const struct node *node);
static inline void node_set_leaf (struct node *node);
static inline void node_set_non_leaf (struct node *node);
static inline uint32_t node_key (const struct node *node, size_t i);
static inline void node_set_key (struct node *node, size_t i, uint32_t key);
static inline uint32_t node_value (const struct node *node, size_t i);
static inline void node_set_value (struct node *node, size_t i,
				   uint32_t value);
static inline uint32_t node_next_leaf (const struct node *node);
static inline void node_set_next_leaf (struct node *node, uint32_t next_leaf);
static inline uint32_t node_parent (const struct node *node);
static inline void node_set_parent (struct node *node, uint32_t parent);
static inline int adftool_bplus_fetch (struct adftool_bplus *bplus,
				       uint32_t row_id,
				       size_t *actual_row_length,
				       size_t request_start,
				       size_t request_length,
				       uint32_t * response);
static inline int node_fetch (struct adftool_bplus *bplus, uint32_t id,
			      struct node *node);
static inline int adftool_bplus_compare (const struct adftool_bplus *bplus,
					 const struct adftool_bplus_key
					 *key_a,
					 const struct adftool_bplus_key
					 *key_b, int *result);
static inline int compare_known (const struct adftool_bplus *bplus,
				 uint32_t key_a, uint32_t key_b, int *result);
static inline void adftool_bplus_allocate (struct adftool_bplus *bplus,
					   uint32_t * node_id);
static inline void adftool_bplus_store (struct adftool_bplus *bplus,
					uint32_t node_id, size_t start,
					size_t length, const uint32_t * row);
static inline void node_store (struct adftool_bplus *bplus,
			       const struct node *node);
static inline void ensure_init (void);

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

static inline void
node_set_leaf (struct node *node)
{
  uint32_t bit = 1;
  uint32_t flag_is_leaf = bit << 31;
  node->row[2 * node->order] = flag_is_leaf;
}

static inline void
node_set_non_leaf (struct node *node)
{
  node->row[2 * node->order] = 0;
}

static inline uint32_t
node_key (const struct node *node, size_t i)
{
  assert (i + 1 < node->order);
  return node->row[i];
}

static inline void
node_set_key (struct node *node, size_t i, uint32_t key)
{
  assert (i + 1 < node->order);
  node->row[i] = key;
}

static inline uint32_t
node_value (const struct node *node, size_t i)
{
  assert (i < node->order);
  return node->row[node->order - 1 + i];
}

static inline void
node_set_value (struct node *node, size_t i, uint32_t value)
{
  assert (i < node->order);
  node->row[node->order - 1 + i] = value;
}

static inline uint32_t
node_next_leaf (const struct node *node)
{
  assert (node_is_leaf (node));
  assert (node->order > 0);
  return node_value (node, node->order - 1);
}

static inline void
node_set_next_leaf (struct node *node, uint32_t next_leaf)
{
  assert (node_is_leaf (node));
  node_set_value (node, node->order - 1, next_leaf);
}

static inline uint32_t
node_parent (const struct node *node)
{
  assert (node->order > 0);
  return node->row[2 * node->order - 1];
}

static inline void
node_set_parent (struct node *node, uint32_t parent)
{
  assert (node->order > 0);
  node->row[2 * node->order - 1] = parent;
}

static void *
appropriate_context_argument (struct context *context)
{
  switch (context->type)
    {
    case UNSET:
      break;
    case LOGICAL:
      return context->arg.logical;
    case HDF5:
      return (void *) (&(context->arg.hdf5));
    }
  assert (0);
  return NULL;
}

static inline int
adftool_bplus_fetch (struct adftool_bplus *bplus,
		     uint32_t row_id, size_t *actual_row_length,
		     size_t request_start, size_t request_length,
		     uint32_t * response)
{
  assert (bplus->fetch != NULL);
  void *ctx = appropriate_context_argument (&(bplus->fetch_context));
  return bplus->fetch (row_id, actual_row_length, request_start,
		       request_length, response, ctx);
}

static inline int
node_fetch (struct adftool_bplus *bplus, uint32_t id, struct node *node)
{
  size_t actual_row_length;
  node->id = id;
  while (1)
    {
      int fetch_error = adftool_bplus_fetch (bplus, node->id,
					     &actual_row_length, 0,
					     2 * node->order + 1,
					     node->row);
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
	    adftool_bplus_fetch (bplus, node->id,
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
adftool_bplus_compare (const struct adftool_bplus *bplus,
		       const struct adftool_bplus_key *key_a,
		       const struct adftool_bplus_key *key_b, int *result)
{
  assert (bplus->compare != NULL);
  return bplus->compare (key_a, key_b, result, bplus->compare_context);
}

static inline int
compare_known (const struct adftool_bplus
	       *bplus, uint32_t key_a, uint32_t key_b, int *result)
{
  struct adftool_bplus_key a, b;
  a.type = ADFTOOL_BPLUS_KEY_KNOWN;
  a.arg.known = key_a;
  b.type = ADFTOOL_BPLUS_KEY_KNOWN;
  b.arg.known = key_b;
  return adftool_bplus_compare (bplus, &a, &b, result);
}

static inline void
adftool_bplus_allocate (struct adftool_bplus *bplus, uint32_t * node_id)
{
  assert (bplus->allocate != NULL);
  void *ctx = appropriate_context_argument (&(bplus->allocate_context));
  bplus->allocate (node_id, ctx);
}

static inline void
adftool_bplus_store (struct adftool_bplus *bplus,
		     uint32_t node_id, size_t start, size_t length,
		     const uint32_t * row)
{
  assert (bplus->store != NULL);
  void *ctx = appropriate_context_argument (&(bplus->store_context));
  bplus->store (node_id, start, length, row, ctx);
}

static inline void
node_store (struct adftool_bplus *bplus, const struct node *node)
{
  adftool_bplus_store (bplus, node->id, 0, 2 * node->order + 1, node->row);
}

void _adftool_ensure_init (void);

static inline void
ensure_init (void)
{
  static volatile int is_initialized = 0;
  if (!is_initialized)
    {
      _adftool_ensure_init ();
      is_initialized = 1;
    }
}

#endif /* not H_ADFTOOL_PRIVATE_INCLUDED */
