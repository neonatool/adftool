#include <adftool_private.h>

struct adftool_bplus_key *
adftool_bplus_key_alloc (void)
{
  ensure_init ();
  struct adftool_bplus_key *ret = malloc (sizeof (struct adftool_bplus_key));
  if (ret != NULL)
    {
      ret->type = ADFTOOL_BPLUS_KEY_UNKNOWN;
      ret->arg.unknown = NULL;
    }
  return ret;
}

void
adftool_bplus_key_free (struct adftool_bplus_key *key)
{
  free (key);
}

int
adftool_bplus_key_get_known (const struct adftool_bplus_key *key,
			     uint32_t * index)
{
  if (key->type == ADFTOOL_BPLUS_KEY_KNOWN)
    {
      *index = key->arg.known;
      return 0;
    }
  return 1;
}

int
adftool_bplus_key_get_unknown (const struct adftool_bplus_key *key,
			       void **key_value)
{
  if (key->type == ADFTOOL_BPLUS_KEY_UNKNOWN)
    {
      *key_value = key->arg.unknown;
      return 0;
    }
  return 1;
}

void
adftool_bplus_key_set_known (struct adftool_bplus_key *key,
			     uint32_t key_index)
{
  key->type = ADFTOOL_BPLUS_KEY_KNOWN;
  key->arg.known = key_index;
}

void
adftool_bplus_key_set_unknown (struct adftool_bplus_key *key, void *key_value)
{
  key->type = ADFTOOL_BPLUS_KEY_UNKNOWN;
  key->arg.unknown = key_value;
}
