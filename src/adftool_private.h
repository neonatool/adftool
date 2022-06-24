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

#endif /* not H_ADFTOOL_PRIVATE_INCLUDED */
