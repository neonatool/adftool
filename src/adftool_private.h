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
#define _(String) gettext (String)
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

static inline int adftool_bplus_fetch (struct adftool_bplus *bplus,
				       uint32_t row_id,
				       size_t *actual_row_length,
				       size_t request_start,
				       size_t request_length,
				       uint32_t * response);
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
static inline void ensure_init (void);

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
