#include <adftool_private.h>

struct adftool_bplus_parameters *
adftool_bplus_parameters_alloc (void)
{
  ensure_init ();
  struct adftool_bplus_parameters *ret =
    malloc (sizeof (struct adftool_bplus_parameters));
  if (ret != NULL)
    {
      ret->fetch = NULL;
      ret->fetch_context.type = ADFTOOL_BPLUS_FETCH_UNSET;
      ret->compare = NULL;
      ret->compare_context = NULL;
    }
  return ret;
}

void
adftool_bplus_parameters_free (struct adftool_bplus_parameters *parameters)
{
  free (parameters);
}

void
adftool_bplus_parameters_set_fetch (struct adftool_bplus_parameters
				    *parameters, int (*fetch) (uint32_t,
							       size_t *,
							       size_t, size_t,
							       uint32_t *,
							       void *),
				    void *context)
{
  parameters->fetch = fetch;
  parameters->fetch_context.type = ADFTOOL_BPLUS_FETCH_LOGICAL;
  parameters->fetch_context.arg.logical = context;
}

void
adftool_bplus_parameters_set_compare (struct adftool_bplus_parameters
				      *parameters,
				      int (*compare) (const struct
						      adftool_bplus_key *,
						      const struct
						      adftool_bplus_key *,
						      int *, void *),
				      void *context)
{
  parameters->compare = compare;
  parameters->compare_context = context;
}

void
adftool_bplus_parameters_set_allocate (struct adftool_bplus_parameters
				       *parameters,
				       void (*allocate) (uint32_t *, void *),
				       void *context)
{
  parameters->allocate = allocate;
  parameters->allocate_context = context;
}

void
adftool_bplus_parameters_set_store (struct adftool_bplus_parameters
				    *parameters,
				    void (*store) (uint32_t, size_t, size_t,
						   const uint32_t *, void *),
				    void *context)
{
  parameters->store = store;
  parameters->store_context = context;
}
