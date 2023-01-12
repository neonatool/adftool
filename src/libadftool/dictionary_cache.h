#ifndef H_ADFTOOL_DICTIONARY_CACHE_INCLUDED
# define H_ADFTOOL_DICTIONARY_CACHE_INCLUDED

# include <adftool.h>
# include <bplus.h>
# include <hdf5.h>

# include "dictionary_strings.h"

# include <stdlib.h>
# include <assert.h>
# include <string.h>
# include <locale.h>
# include <stdbool.h>

# include "gettext.h"

# ifdef BUILDING_LIBADFTOOL
#  define _(String) dgettext (PACKAGE, (String))
#  define N_(String) (String)
# else
#  define _(String) gettext (String)
#  define N_(String) (String)
# endif

struct adftool_dictionary_cache;

static struct adftool_dictionary_cache *adftool_dictionary_cache_alloc (size_t
									n_entries,
									size_t
									max_entries_length);
static void adftool_dictionary_cache_free (struct adftool_dictionary_cache
					   *cache);

static int adftool_dictionary_cache_setup (struct adftool_dictionary_cache
					   *cache, hid_t file);

static int adftool_dictionary_cache_get_a (struct adftool_dictionary_cache
					   *cache, uint32_t id,
					   size_t *length, char **data);

static int adftool_dictionary_cache_add (struct adftool_dictionary_cache
					 *cache, uint32_t length,
					 const char *data, uint32_t * id);

struct adftool_dictionary_cache_entry
{
  uint32_t id;
  size_t length;
  /* data = NULL is the sole indicator that the entry is free */
  char *data;
};

struct adftool_dictionary_cache
{
  size_t n_entries;
  size_t maximum_entry_length;
  struct adftool_dictionary_cache_entry *entries;
  struct adftool_dictionary_strings *strings;
};

static struct adftool_dictionary_cache *
adftool_dictionary_cache_alloc (size_t n_entries, size_t max_entry_length)
{
  struct adftool_dictionary_cache *ret =
    malloc (sizeof (struct adftool_dictionary_cache));
  if (ret != NULL)
    {
      ret->n_entries = n_entries;
      ret->maximum_entry_length = max_entry_length;
      ret->entries =
	malloc (n_entries * sizeof (struct adftool_dictionary_cache_entry));
      ret->strings = adftool_dictionary_strings_alloc ();
      if (ret->entries == NULL || ret->strings == NULL)
	{
	  free (ret->entries);
	  adftool_dictionary_strings_free (ret->strings);
	  free (ret);
	  ret = NULL;
	}
    }
  if (ret != NULL)
    {
      for (size_t i = 0; i < n_entries; i++)
	{
	  ret->entries[i].data = NULL;
	}
    }
  return ret;
}

static void
adftool_dictionary_cache_free (struct adftool_dictionary_cache *cache)
{
  if (cache != NULL)
    {
      for (size_t i = 0; i < cache->n_entries; i++)
	{
	  free (cache->entries[i].data);
	  cache->entries[i].data = NULL;
	}
      free (cache->entries);
      adftool_dictionary_strings_free (cache->strings);
    }
  free (cache);
}

static int
adftool_dictionary_cache_setup (struct adftool_dictionary_cache *cache,
				hid_t file)
{
  for (size_t i = 0; i < cache->n_entries; i++)
    {
      free (cache->entries[i].data);
      cache->entries[i].data = NULL;
    }
  return adftool_dictionary_strings_setup (cache->strings, file);
}

static size_t
adftool_dictionary_cache_hash_id (const struct adftool_dictionary_cache
				  *cache, uint32_t id)
{
  /* FIXME: this hash function is awful */
  return id % cache->n_entries;
}

static int
adftool_dictionary_cache_get_a (struct adftool_dictionary_cache *cache,
				uint32_t id, size_t *length, char **data)
{
  size_t i = adftool_dictionary_cache_hash_id (cache, id);
  if (cache->entries[i].data != NULL && cache->entries[i].id == id)
    {
      *length = cache->entries[i].length;
      *data = malloc (*length + 1);
      if (*data == NULL)
	{
	  return 1;
	}
      memcpy (*data, cache->entries[i].data, cache->entries[i].length);
      (*data)[cache->entries[i].length] = '\0';
    }
  else
    {
      free (cache->entries[i].data);
      cache->entries[i].id = id;
      int error = adftool_dictionary_strings_get_a (cache->strings, id,
						    &(cache->entries[i].
						      length),
						    &(cache->entries[i].
						      data));
      if (error != 0)
	{
	  cache->entries[i].data = NULL;
	  return error;
	}
      *length = cache->entries[i].length;
      *data = malloc (*length + 1);
      if (*data == NULL)
	{
	  return 1;
	}
      memcpy (*data, cache->entries[i].data, cache->entries[i].length);
      (*data)[cache->entries[i].length] = '\0';
    }
  return 0;
}

static int
adftool_dictionary_cache_add (struct adftool_dictionary_cache *cache,
			      uint32_t length, const char *data,
			      uint32_t * id)
{
  /* It’s not a problem if we don’t update the cache yet. */
  return adftool_dictionary_strings_add (cache->strings, length, data, id);
}

#endif /* not H_ADFTOOL_DICTIONARY_CACHE_INCLUDED */
