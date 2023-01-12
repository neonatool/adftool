#ifndef H_ADFTOOL_QUADS_INDEX_INCLUDED
# define H_ADFTOOL_QUADS_INDEX_INCLUDED

# include <adftool.h>
# include <bplus.h>
# include <hdf5.h>

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

struct adftool_quads_index;

static struct adftool_quads_index *adftool_quads_index_alloc (hid_t file,
							      size_t
							      default_order,
							      const char
							      *order,
							      struct
							      adftool_quads
							      *data);
static void adftool_quads_index_free (struct adftool_quads_index *index);

static int
adftool_quads_index_find (struct adftool_quads_index *index,
			  struct adftool_quads *quads,
			  struct adftool_dictionary_index *dictionary,
			  const struct adftool_statement *pattern,
			  int (*iterate) (void *context, size_t n,
					  const uint32_t * ids,
					  const struct adftool_statement **
					  results), void *iterator_context);

static int
adftool_quads_index_insert (struct adftool_quads_index *index,
			    struct adftool_quads *quads,
			    struct adftool_dictionary_index *dictionary,
			    const struct adftool_statement *statement,
			    uint32_t id);

struct adftool_quads_index
{
  struct bplus_hdf5_table *handle;
  struct bplus_tree *tree;
  struct adftool_quads *data;
  char order[5];
};

static bool
check_order_string (const char *order)
{
  bool has_s = false;
  bool has_p = false;
  bool has_o = false;
  bool has_g = false;
  for (size_t i = 0; i < strlen (order); i++)
    {
      bool *relevant = NULL;
      switch (order[i])
	{
	case 'S':
	  relevant = &has_s;
	  break;
	case 'P':
	  relevant = &has_p;
	  break;
	case 'O':
	  relevant = &has_o;
	  break;
	case 'G':
	  relevant = &has_g;
	  break;
	default:
	  return false;
	}
      if (*relevant)
	{
	  /* Already seen. */
	  return false;
	}
      *relevant = true;
    }
  return (has_s && has_p && has_o && has_g);
}

static struct adftool_quads_index *
adftool_quads_index_alloc (hid_t file, size_t default_order,
			   const char *order, struct adftool_quads *data)
{
  struct adftool_quads_index *ret = NULL;
  if (check_order_string (order))
    {
      ret = malloc (sizeof (struct adftool_quads_index));
      if (ret != NULL)
	{
	  assert (strlen (order) == 4);
	  strcpy (ret->order, order);
	}
    }
  if (ret != NULL)
    {
      ret->data = data;
      ret->handle = bplus_hdf5_table_alloc ();
      if (ret->handle != NULL)
	{
	  char name[] = "/data-description/index_????";
	  sprintf (name, "/data-description/index_%s", order);
	  hid_t dataset = H5Dopen2 (file, name, H5P_DEFAULT);
	  if (dataset == H5I_INVALID_HID)
	    {
	      hsize_t minimum_dimensions[] = { 0, 2 * default_order + 1 };
	      hsize_t maximum_dimensions[] =
		{ H5S_UNLIMITED, 2 * default_order + 1 };
	      hsize_t chunk_dimensions[] = { 1, 2 * default_order + 1 };
	      hid_t fspace =
		H5Screate_simple (2, minimum_dimensions, maximum_dimensions);
	      hid_t dataset_creation_properties =
		H5Pcreate (H5P_DATASET_CREATE);
	      hid_t link_creation_properties = H5Pcreate (H5P_LINK_CREATE);
	      if (fspace == H5I_INVALID_HID
		  || dataset_creation_properties == H5I_INVALID_HID
		  || link_creation_properties == H5I_INVALID_HID)
		{
		  H5Sclose (fspace);
		  H5Pclose (dataset_creation_properties);
		  H5Pclose (link_creation_properties);
		  bplus_hdf5_table_free (ret->handle);
		  free (ret);
		  return NULL;
		}
	      if ((H5Pset_chunk
		   (dataset_creation_properties, 2, chunk_dimensions) < 0)
		  ||
		  (H5Pset_create_intermediate_group
		   (link_creation_properties, 1) < 0)
		  ||
		  ((dataset =
		    H5Dcreate2 (file, name,
				bplus_hdf5_table_type (ret->handle), fspace,
				link_creation_properties,
				dataset_creation_properties,
				H5P_DEFAULT)) == H5I_INVALID_HID))
		{
		  H5Sclose (fspace);
		  H5Pclose (dataset_creation_properties);
		  H5Pclose (link_creation_properties);
		  H5Dclose (dataset);
		  bplus_hdf5_table_free (ret->handle);
		  free (ret);
		  return NULL;
		}
	      H5Sclose (fspace);
	      H5Pclose (dataset_creation_properties);
	      H5Pclose (link_creation_properties);
	    }
	  int error = bplus_hdf5_table_set (ret->handle, dataset);
	  if (error)
	    {
	      /* dataset has already been taken care of. */
	      bplus_hdf5_table_free (ret->handle);
	      free (ret);
	      return NULL;
	    }
	}
      ret->tree = NULL;
      if (ret->handle != NULL)
	{
	  ret->tree = bplus_tree_alloc (bplus_hdf5_table_order (ret->handle));
	}
      if (ret->handle == NULL || ret->data == NULL || ret->tree == NULL)
	{
	  bplus_hdf5_table_free (ret->handle);
	  bplus_tree_free (ret->tree);
	  free (ret);
	  ret = NULL;
	}
    }
  return ret;
}

static void
adftool_quads_index_free (struct adftool_quads_index *index)
{
  if (index != NULL)
    {
      bplus_hdf5_table_free (index->handle);
      bplus_tree_free (index->tree);
    }
  free (index);
}

static inline int
adftool_quads_index_unpack_key (struct adftool_quads *quads,
				struct adftool_dictionary_index *dictionary,
				const struct bplus_key *key,
				struct adftool_statement *statement)
{
  switch (key->type)
    {
    case BPLUS_KEY_KNOWN:
      return quads_get (quads, dictionary, key->arg.known, statement);
    case BPLUS_KEY_UNKNOWN:
      {
	const struct adftool_statement *pattern = key->arg.unknown;
	statement_copy (statement, pattern);
	return 0;
      }
    }
  assert (0);
  return 42;
}

struct adftool_quads_index_file
{
  struct adftool_quads *quads;
  struct adftool_dictionary_index *dictionary;
  struct adftool_quads_index *index;
};

static inline int
adftool_quads_index_compare (void *context, const struct bplus_key *a,
			     const struct bplus_key *b, int *result)
{
  struct adftool_quads_index_file *file = context;
  struct adftool_statement *sa = adftool_statement_alloc ();
  struct adftool_statement *sb = adftool_statement_alloc ();
  if (sa == NULL || sb == NULL)
    {
      adftool_statement_free (sa);
      adftool_statement_free (sb);
      return 1;
    }
  int error_a =
    adftool_quads_index_unpack_key (file->quads, file->dictionary, a, sa);
  int error_b =
    adftool_quads_index_unpack_key (file->quads, file->dictionary, b, sb);
  if (error_a || error_b)
    {
      adftool_statement_free (sa);
      adftool_statement_free (sb);
      return 1;
    }
  *result = statement_compare (sa, sb, file->index->order);
  adftool_statement_free (sa);
  adftool_statement_free (sb);
  return 0;
}

struct adftool_quads_index_decision_ctx
{
  struct adftool_quads_index *index;
  bool decided_to_abort;
  uint32_t id;
};

static inline int
adftool_quads_index_decide (void *context, int present,
			    const struct bplus_key *key, uint32_t * id,
			    int *back)
{
  (void) key;
  struct adftool_quads_index_decision_ctx *decision = context;
  if (present)
    {
      decision->decided_to_abort = true;
      return 1;
    }
  decision->decided_to_abort = false;
  *id = decision->id;
  *back = 0;
  return 0;
}

struct adftool_quads_index_iterator_ctx
{
  int (*iterate_over_statements) (void *, size_t, const uint32_t *,
				  const struct adftool_statement **);
  void *context;
  struct adftool_quads *quads;
  struct adftool_dictionary_index *dictionary;
};

static inline int
adftool_quads_index_iterate (void *ctx, size_t n,
			     const struct bplus_key *keys,
			     const uint32_t * values)
{
  int error = 0;
  (void) keys;
  struct adftool_quads_index_iterator_ctx *context = ctx;
  struct adftool_statement **statements =
    malloc (n * sizeof (struct adftool_statement *));
  if (statements == NULL)
    {
      error = 1;
      goto cleanup;
    }
  for (size_t i = 0; i < n; i++)
    {
      statements[i] = NULL;
    }
  for (size_t i = 0; i < n; i++)
    {
      statements[i] = statement_alloc ();
      if (statements[i] == NULL)
	{
	  error = 1;
	  goto cleanup_statements;
	}
      int get_error =
	quads_get (context->quads, context->dictionary, values[i],
		   statements[i]);
      if (get_error)
	{
	  error = 1;
	  goto cleanup_statements;
	}
    }
  error =
    context->iterate_over_statements (context->context, n, values,
				      (const struct adftool_statement **)
				      statements);
cleanup_statements:
  for (size_t i = 0; i < n; i++)
    {
      statement_free (statements[i]);
    }
  free (statements);
cleanup:
  return error;
}

static int
adftool_quads_index_find (struct adftool_quads_index *index,
			  struct adftool_quads *quads,
			  struct adftool_dictionary_index *dictionary,
			  const struct adftool_statement *pattern,
			  int (*iterate) (void *context, size_t n,
					  const uint32_t * ids,
					  const struct adftool_statement **
					  results), void *iterator_context)
{
  struct adftool_quads_index_file compare_context;
  compare_context.quads = quads;
  compare_context.dictionary = dictionary;
  compare_context.index = index;
  struct adftool_quads_index_iterator_ctx it_context;
  it_context.iterate_over_statements = iterate;
  it_context.context = iterator_context;
  it_context.quads = quads;
  it_context.dictionary = dictionary;
  struct bplus_key unknown;
  unknown.type = BPLUS_KEY_UNKNOWN;
  unknown.arg.unknown = (void *) pattern;
  int error = bplus_find (index->tree, bplus_hdf5_fetch, index->handle,
			  adftool_quads_index_compare, &compare_context,
			  adftool_quads_index_iterate, &it_context,
			  &unknown);
  return error;
}

static int
adftool_quads_index_insert (struct adftool_quads_index *index,
			    struct adftool_quads *quads,
			    struct adftool_dictionary_index *dictionary,
			    const struct adftool_statement *statement,
			    uint32_t id)
{
  struct adftool_quads_index_file compare_context;
  compare_context.quads = quads;
  compare_context.dictionary = dictionary;
  compare_context.index = index;
  struct adftool_quads_index_decision_ctx decision_context;
  decision_context.index = index;
  decision_context.decided_to_abort = false;
  decision_context.id = id;
  struct bplus_key unknown;
  unknown.type = BPLUS_KEY_UNKNOWN;
  unknown.arg.unknown = (void *) statement;
  int error = bplus_insert (index->tree, bplus_hdf5_fetch, index->handle,
			    adftool_quads_index_compare, &compare_context,
			    bplus_hdf5_allocate, index->handle,
			    bplus_hdf5_update,
			    index->handle, adftool_quads_index_decide,
			    &decision_context, &unknown);
  if (error && decision_context.decided_to_abort)
    {
      return 0;
    }
  return error;
}

#endif /* not H_ADFTOOL_QUADS_INDEX_INCLUDED */
