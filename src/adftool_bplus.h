#ifndef H_ADFTOOL_BPLUS_INCLUDED
#define H_ADFTOOL_BPLUS_INCLUDED

#include <adftool_private.h>

#include <hdf5.h>

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

struct adftool_bplus_key;

struct bplus
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

static inline void bplus_set_fetch (struct bplus *bplus,
				    int (*fetch) (uint32_t, size_t *, size_t,
						  size_t, uint32_t *, void *),
				    void *context);

static inline void bplus_set_compare (struct bplus *bplus,
				      int (*compare) (const struct
						      adftool_bplus_key *,
						      const struct
						      adftool_bplus_key *,
						      int *, void *),
				      void *context);

  /* The allocation and storage should not signal errors, because
     there’s no way to recover from a partially failing update. If you
     cannot guarantee error-free updates in the backend, store the
     updates in a cache and do them all at once. If you do that, make
     sure to also tweak the "fetch" callback to first look in the
     cache. */

static inline void bplus_set_allocate (struct bplus *bplus,
				       void (*allocate) (uint32_t *, void *),
				       void *context);

static inline void bplus_set_store (struct bplus *bplus,
				    void (*store) (uint32_t, size_t, size_t,
						   const uint32_t *, void *),
				    void *context);

static inline int bplus_from_hdf5 (struct bplus *bplus, hid_t dataset,
				   hid_t next_id_attribute);

  /* The lookup function. The parameters "fetch" and "compare" must be
     set. */

static inline int bplus_lookup (const struct adftool_bplus_key *needle,
				struct bplus *bplus, size_t start, size_t max,
				size_t *n_results, uint32_t * results);

  /* Add a parent to the root. The parameters "fetch", "allocate" and
     "store" must be set. */

static inline int bplus_grow (struct bplus *bplus);

  /* The insert function requires "fetch", "compare", "allocate" and
     "store" parameters.
   */
static inline int bplus_insert (uint32_t key, uint32_t value,
				struct bplus *bplus);

static inline int bplus_fetch (struct bplus *bplus, uint32_t row_id,
			       size_t *actual_row_length,
			       size_t request_start, size_t request_length,
			       uint32_t * response);
static inline int bplus_compare (const struct bplus *bplus,
				 const struct adftool_bplus_key *key_a,
				 const struct adftool_bplus_key *key_b,
				 int *result);
static inline int bplus_compare_known (const struct bplus *bplus,
				       uint32_t key_a, uint32_t key_b,
				       int *result);
static inline void bplus_allocate (struct bplus *bplus, uint32_t * node_id);
static inline void bplus_store (struct bplus *bplus, uint32_t node_id,
				size_t start, size_t length,
				const uint32_t * row);

#include "adftool_bplus_key.h"

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
bplus_fetch (struct bplus *bplus, uint32_t row_id, size_t *actual_row_length,
	     size_t request_start, size_t request_length, uint32_t * response)
{
  assert (bplus->fetch != NULL);
  void *ctx = appropriate_context_argument (&(bplus->fetch_context));
  return bplus->fetch (row_id, actual_row_length, request_start,
		       request_length, response, ctx);
}

static inline int
bplus_compare (const struct bplus *bplus,
	       const struct adftool_bplus_key *key_a,
	       const struct adftool_bplus_key *key_b, int *result)
{
  assert (bplus->compare != NULL);
  return bplus->compare (key_a, key_b, result, bplus->compare_context);
}

static inline int
bplus_compare_known (const struct bplus *bplus, uint32_t key_a,
		     uint32_t key_b, int *result)
{
  struct adftool_bplus_key a, b;
  a.type = KEY_KNOWN;
  a.arg.known = key_a;
  b.type = KEY_KNOWN;
  b.arg.known = key_b;
  return bplus_compare (bplus, &a, &b, result);
}

static inline void
bplus_allocate (struct bplus *bplus, uint32_t * node_id)
{
  assert (bplus->allocate != NULL);
  void *ctx = appropriate_context_argument (&(bplus->allocate_context));
  bplus->allocate (node_id, ctx);
}

static inline void
bplus_store (struct bplus *bplus, uint32_t node_id, size_t start,
	     size_t length, const uint32_t * row)
{
  assert (bplus->store != NULL);
  void *ctx = appropriate_context_argument (&(bplus->store_context));
  bplus->store (node_id, start, length, row, ctx);
}

static inline void
bplus_set_fetch (struct bplus *bplus,
		 int (*fetch) (uint32_t, size_t *, size_t, size_t, uint32_t *,
			       void *), void *context)
{
  bplus->fetch = fetch;
  bplus->fetch_context.type = LOGICAL;
  bplus->fetch_context.arg.logical = context;
}

static inline void
bplus_set_compare (struct bplus *bplus,
		   int (*compare) (const struct adftool_bplus_key *,
				   const struct adftool_bplus_key *, int *,
				   void *), void *context)
{
  bplus->compare = compare;
  bplus->compare_context = context;
}

static inline void
bplus_set_allocate (struct bplus *bplus,
		    void (*allocate) (uint32_t *, void *), void *context)
{
  bplus->allocate = allocate;
  bplus->allocate_context.type = LOGICAL;
  bplus->allocate_context.arg.logical = context;
}

static inline void
bplus_set_store (struct bplus *bplus,
		 void (*store) (uint32_t, size_t, size_t, const uint32_t *,
				void *), void *context)
{
  bplus->store = store;
  bplus->store_context.type = LOGICAL;
  bplus->store_context.arg.logical = context;
}

#include <adftool_hdf5.h>
#include <adftool_grow.h>
#include <adftool_lookup.h>
#include <adftool_insert.h>

#endif /* not H_ADFTOOL_BPLUS_INCLUDED */
