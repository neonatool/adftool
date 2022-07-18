#include <adftool_private.h>

struct adftool_bplus *
adftool_bplus_alloc (void)
{
  ensure_init ();
  struct adftool_bplus *ret = malloc (sizeof (struct adftool_bplus));
  if (ret != NULL)
    {
      ret->fetch = NULL;
      ret->fetch_context.type = UNSET;
      ret->compare = NULL;
      ret->compare_context = NULL;
      ret->allocate = NULL;
      ret->allocate_context.type = UNSET;
      ret->store = NULL;
      ret->store_context.type = UNSET;
    }
  return ret;
}

void
adftool_bplus_free (struct adftool_bplus *bplus)
{
  free (bplus);
}

void
adftool_bplus_set_fetch (struct adftool_bplus *bplus,
			 int (*fetch) (uint32_t, size_t *, size_t, size_t,
				       uint32_t *, void *), void *context)
{
  bplus->fetch = fetch;
  bplus->fetch_context.type = LOGICAL;
  bplus->fetch_context.arg.logical = context;
}

void
adftool_bplus_set_compare (struct adftool_bplus *bplus,
			   int (*compare) (const struct adftool_bplus_key *,
					   const struct adftool_bplus_key *,
					   int *, void *), void *context)
{
  bplus->compare = compare;
  bplus->compare_context = context;
}

void
adftool_bplus_set_allocate (struct adftool_bplus *bplus,
			    void (*allocate) (uint32_t *, void *),
			    void *context)
{
  bplus->allocate = allocate;
  bplus->allocate_context.type = LOGICAL;
  bplus->allocate_context.arg.logical = context;
}

void
adftool_bplus_set_store (struct adftool_bplus *bplus,
			 void (*store) (uint32_t, size_t, size_t,
					const uint32_t *, void *),
			 void *context)
{
  bplus->store = store;
  bplus->store_context.type = LOGICAL;
  bplus->store_context.arg.logical = context;
}
