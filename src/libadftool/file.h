#ifndef H_ADFTOOL_FILE_INCLUDED
# define H_ADFTOOL_FILE_INCLUDED

# include <adftool.h>
# include <bplus.h>
# include <hdf5.h>

# include "dictionary_index.h"
# include "quads.h"
# include "quads_index.h"

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

static inline void ensure_init (void);

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

struct adftool_file;

static inline
  struct adftool_file *adftool_file_alloc (hid_t hdf5_file,
					   FILE * stdio_file,
					   size_t default_order,
					   size_t n_cache_entries,
					   size_t max_cache_length);
static inline void adftool_file_free (struct adftool_file *cache);

static int adftool_file_lookup (struct adftool_file *file,
				const struct adftool_statement *pattern,
				int (*iterate) (void *context, size_t n,
						const struct adftool_statement
						** results),
				void *iterator_context);

static inline
  int adftool_file_delete (struct adftool_file *file,
			   const struct adftool_statement *pattern,
			   uint64_t deletion_date);

static inline
  int adftool_file_insert (struct adftool_file *file,
			   const struct adftool_statement *statement);

struct adftool_file
{
  hid_t hdf5_handle;
  FILE *file_handle;
  struct adftool_dictionary_index *dictionary;
  struct adftool_quads *quads;
  struct adftool_quads_index *indices[6];
};

static inline struct adftool_file *
adftool_file_alloc (hid_t file, FILE * stdio_file, size_t default_order,
		    size_t n_cache_entries, size_t max_cache_length)
{
  struct adftool_file *ret = malloc (sizeof (struct adftool_file));
  if (ret == NULL)
    {
      goto error;
    }
  ret->hdf5_handle = file;
  ret->file_handle = stdio_file;
  if (file == H5I_INVALID_HID)
    {
      goto cleanup;
    }
  ret->dictionary =
    adftool_dictionary_index_alloc (file, default_order, n_cache_entries,
				    max_cache_length);
  if (ret->dictionary == NULL)
    {
      goto cleanup;
    }
  ret->quads = adftool_quads_alloc ();
  if (ret->quads == NULL)
    {
      goto cleanup_dictionary;
    }
  if (adftool_quads_setup (ret->quads, file) != 0)
    {
      goto cleanup_quads;
    }
  static const char orders[][6] = {
    "GSPO", "GPOS", "GOSP", "SPOG", "POSG", "OSPG"
  };
  for (size_t i = 0; i < 6; i++)
    {
      ret->indices[i] = NULL;
    }
  for (size_t i = 0; i < sizeof (orders) / sizeof (orders[0]); i++)
    {
      ret->indices[i] =
	adftool_quads_index_alloc (file, default_order, orders[i],
				   ret->quads);
      if (ret->indices[i] == NULL)
	{
	  goto cleanup_quad_indices;
	}
    }
  return ret;
cleanup_quad_indices:
  for (size_t i = 0; i < 6; i++)
    {
      adftool_quads_index_free (ret->indices[i]);
    }
cleanup_quads:
  adftool_quads_free (ret->quads);
cleanup_dictionary:
  adftool_dictionary_index_free (ret->dictionary);
cleanup:
  free (ret);
error:
  return NULL;
}

static void
adftool_file_free (struct adftool_file *file)
{
  if (file != NULL)
    {
      for (size_t i = 0; i < 6; i++)
	{
	  adftool_quads_index_free (file->indices[i]);
	}
      adftool_quads_free (file->quads);
      adftool_dictionary_index_free (file->dictionary);
      H5Fclose (file->hdf5_handle);
      fclose (file->file_handle);
    }
  free (file);
}

static bool
adftool_file_can_use_index (const struct adftool_statement *pattern,
			    const char *index_order)
{
  /* For an index to be usable, it must represent all set terms in the
     pattern first, and then all wildcard terms. */
  bool term_can_be_set = true;
  for (size_t i = 0; i < strlen (index_order); i++)
    {
      const struct adftool_term *t = NULL;
      switch (index_order[i])
	{
	case 'S':
	  t = pattern->subject;
	  break;
	case 'P':
	  t = pattern->predicate;
	  break;
	case 'O':
	  t = pattern->object;
	  break;
	case 'G':
	  t = pattern->graph;
	  break;
	default:
	  assert (0);
	}
      if (t == NULL)
	{
	  /* From now on, all other terms in the pattern MUST be
	     unset. */
	  term_can_be_set = false;
	}
      else if (!term_can_be_set)
	{
	  /* There is a hole in that pattern. */
	  return false;
	}
    }
  return true;
}

struct adftool_file_lookup_iterator_ctx
{
  int (*iterate) (void *context, size_t n,
		  const struct adftool_statement ** results);
  void *context;
};

static inline int
adftool_file_lookup_iterator (void *ctx, size_t n, const uint32_t * ids,
			      const struct adftool_statement **result)
{
  struct adftool_file_lookup_iterator_ctx *iterator = ctx;
  (void) ids;
  return iterator->iterate (iterator->context, n, result);
}

static int
adftool_file_lookup (struct adftool_file *file,
		     const struct adftool_statement *pattern,
		     int (*iterate) (void *context, size_t n,
				     const struct adftool_statement **
				     results), void *iterator_context)
{
  struct adftool_file_lookup_iterator_ctx ctx = {
    .iterate = iterate,
    .context = iterator_context
  };
  for (size_t i = 0; i < 6; i++)
    {
      if (adftool_file_can_use_index (pattern, file->indices[i]->order))
	{
	  return adftool_quads_index_find (file->indices[i], file->quads,
					   file->dictionary, pattern,
					   adftool_file_lookup_iterator,
					   &ctx);
	}
    }
  assert (0);
  return 42;
}

struct adftool_file_deletion_iterator_ctx
{
  struct adftool_file *file;
  uint64_t deletion_date;
};

static inline int
adftool_file_deletion_iterator (void *ctx, size_t n, const uint32_t * ids,
				const struct adftool_statement **result)
{
  (void) result;
  struct adftool_file_deletion_iterator_ctx *deletion = ctx;
  int any_error = 0;
  for (size_t i = 0; i < n; i++)
    {
      int error =
	quads_delete (deletion->file->quads, deletion->file->dictionary,
		      ids[i], deletion->deletion_date);
      if (error)
	{
	  any_error = 1;
	}
    }
  return any_error;
}

static inline int
adftool_file_delete (struct adftool_file *file,
		     const struct adftool_statement *pattern,
		     uint64_t deletion_date)
{
  struct adftool_file_deletion_iterator_ctx ctx = {
    .file = file,
    .deletion_date = deletion_date
  };
  for (size_t i = 0; i < 6; i++)
    {
      if (adftool_file_can_use_index (pattern, file->indices[i]->order))
	{
	  return adftool_quads_index_find (file->indices[i], file->quads,
					   file->dictionary, pattern,
					   adftool_file_deletion_iterator,
					   &ctx);
	}
    }
  assert (0);
  return 42;
}

struct adftool_file_insertion_ctx
{
  bool cancel;
};

static inline int
adftool_file_insert_iterator (void *context, size_t n,
			      const struct adftool_statement **results)
{
  (void) results;
  struct adftool_file_insertion_ctx *ctx = context;
  ctx->cancel = ctx->cancel || (n != 0);
  return 0;
}

static inline int
adftool_file_insert (struct adftool_file *file,
		     const struct adftool_statement *pattern)
{
  assert (pattern->subject != NULL);
  assert (pattern->predicate != NULL);
  assert (pattern->object != NULL);
  struct adftool_file_insertion_ctx ctx = {
    .cancel = false
  };
  int error =
    adftool_file_lookup (file, pattern, adftool_file_insert_iterator, &ctx);
  if (error)
    {
      return error;
    }
  if (ctx.cancel)
    {
      return 0;
    }
  /* The statement is not already present. */
  uint32_t new_id;
  int insertion_error =
    quads_insert (file->quads, file->dictionary, pattern, &new_id);
  if (insertion_error)
    {
      return 1;
    }
  for (size_t i = 0; i < 6; i++)
    {
      int err = adftool_quads_index_insert (file->indices[i], file->quads,
					    file->dictionary, pattern,
					    new_id);
      insertion_error = insertion_error || err;
    }
  return insertion_error;
}

#endif /* not H_ADFTOOL_FILE_INCLUDED */
