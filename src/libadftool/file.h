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

# define DEALLOC_FILE \
  ATTRIBUTE_DEALLOC (adftool_file_free, 1)

struct adftool_file;

MAYBE_UNUSED static void adftool_file_free (struct adftool_file *cache);

MAYBE_UNUSED DEALLOC_FILE
  static
  struct adftool_file *adftool_file_alloc (hid_t hdf5_file,
					   size_t default_order,
					   size_t n_cache_entries,
					   size_t max_cache_length);

MAYBE_UNUSED DEALLOC_FILE
  static struct adftool_file *file_open (const char *filename, bool write);

MAYBE_UNUSED DEALLOC_FILE
  static struct adftool_file
  *file_open_data (size_t n_bytes, const void *bytes);

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
  struct adftool_dictionary_index *dictionary;
  struct adftool_quads *quads;
  struct adftool_quads_index *indices[6];
};

static struct adftool_file *
adftool_file_alloc (hid_t file, size_t default_order,
		    size_t n_cache_entries, size_t max_cache_length)
{
  struct adftool_file *ret = malloc (sizeof (struct adftool_file));
  if (ret == NULL)
    {
      goto error;
    }
  ret->hdf5_handle = file;
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

# define DEFAULT_ORDER 256
# define DEFAULT_DICTIONARY_CACHE_ENTRIES 8191
# define DEFAULT_DICTIONARY_CACHE_ENTRY_LENGTH 512

# if defined _WIN32
#  define OPEN_BINARY_SUFFIX "b"
# else
#  define OPEN_BINARY_SUFFIX ""
# endif

static struct adftool_file *
file_open (const char *filename, bool write)
{
  hid_t hdf5_file = H5I_INVALID_HID;
  unsigned mode = H5F_ACC_RDONLY;
  if (write)
    {
      mode = H5F_ACC_RDWR;
    }
  hdf5_file = H5Fopen (filename, mode, H5P_DEFAULT);
  if (hdf5_file == H5I_INVALID_HID)
    {
      if (write)
	{
	  /* Try creating it… */
	  hdf5_file =
	    H5Fcreate (filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
	}
      if (hdf5_file == H5I_INVALID_HID)
	{
	  return NULL;
	}
    }
  struct adftool_file *ret = adftool_file_alloc (hdf5_file, DEFAULT_ORDER,
						 DEFAULT_DICTIONARY_CACHE_ENTRIES,
						 DEFAULT_DICTIONARY_CACHE_ENTRY_LENGTH);
  if (ret == NULL)
    {
      H5Fclose (hdf5_file);
    }
  return ret;
}

static struct adftool_file *
file_open_data (size_t n_bytes, const void *bytes)
{
  /* Create a temporary file initially containing these bytes, open
     the HDF5 file, and then unlink the file. So, the HDF5 file name
     will not exist. To let users read the generated file, the
     file_open function also opens it with a regular FILE* handle. */
  const char *tmpdir_var = getenv ("TMPDIR");
  static const char *default_tmpdir = "/tmp";
  struct adftool_file *ret = NULL;
  if (tmpdir_var == NULL)
    {
      tmpdir_var = default_tmpdir;
    }
  char *filename =
    malloc (strlen (tmpdir_var) + strlen ("/adftool-XXXXXX") + 1);
  if (filename == NULL)
    {
      goto wrapup;
    }
  sprintf (filename, "%s/adftool-XXXXXX", tmpdir_var);
  int descr = mkstemp (filename);
  if (descr == -1)
    {
      goto free_filename;
    }
  FILE *f = fopen (filename, "w" OPEN_BINARY_SUFFIX);
  if (f == NULL)
    {
      goto close_descriptor;
    }
  size_t n_written = 0;
  if (n_bytes != 0)
    {
      n_written = fwrite (bytes, 1, n_bytes, f);
    }
  if (n_written != n_bytes)
    {
      goto close_file;
    }
  if (fflush (f) != 0)
    {
      goto close_file;
    }
  ret = file_open (filename, 1);
  if (ret == NULL)
    {
      goto close_file;
    }
close_file:
  fclose (f);
close_descriptor:
  remove (filename);
  close (descr);
free_filename:
  free (filename);
wrapup:
  return ret;
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
