#ifndef H_ADFTOOL_DICTIONARY_INDEX_INCLUDED
# define H_ADFTOOL_DICTIONARY_INDEX_INCLUDED

# include <adftool.h>
# include <bplus.h>
# include <hdf5.h>

# include "dictionary_cache.h"

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

struct adftool_dictionary_index;

static inline
  struct adftool_dictionary_index *adftool_dictionary_index_alloc (hid_t
								   file,
								   size_t
								   default_order,
								   size_t
								   n_cache_entries,
								   size_t
								   max_cache_length);
static inline
  void adftool_dictionary_index_free (struct adftool_dictionary_index *index);

static
  int adftool_dictionary_index_find (struct adftool_dictionary_index
				     *index, size_t length,
				     const char *key,
				     bool insert_if_missing, int *found,
				     uint32_t * id);

struct adftool_dictionary_index
{
  struct bplus_hdf5_table *handle;
  struct bplus_tree *tree;
  struct adftool_dictionary_cache *data;
};

static inline struct adftool_dictionary_index *
adftool_dictionary_index_alloc (hid_t file, size_t default_order,
				size_t n_cache_entries,
				size_t max_cache_length)
{
  struct adftool_dictionary_index *ret =
    malloc (sizeof (struct adftool_dictionary_index));
  if (ret != NULL)
    {
      ret->handle = bplus_hdf5_table_alloc ();
      if (ret->handle != NULL)
	{
	  hid_t dataset = H5Dopen2 (file, "/dictionary/keys", H5P_DEFAULT);
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
		    H5Dcreate2 (file, "/dictionary/keys", H5T_STD_U32BE,
				fspace, link_creation_properties,
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
      ret->data =
	adftool_dictionary_cache_alloc (n_cache_entries, max_cache_length);
      if (ret->data != NULL)
	{
	  int setup_error = adftool_dictionary_cache_setup (ret->data, file);
	  if (setup_error)
	    {
	      adftool_dictionary_cache_free (ret->data);
	      ret->data = NULL;
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
	  adftool_dictionary_cache_free (ret->data);
	  free (ret);
	  ret = NULL;
	}
    }
  return ret;
}

static inline void
adftool_dictionary_index_free (struct adftool_dictionary_index *index)
{
  if (index != NULL)
    {
      bplus_hdf5_table_free (index->handle);
      bplus_tree_free (index->tree);
      adftool_dictionary_cache_free (index->data);
    }
  free (index);
}

struct adftool_dictionary_index_key
{
  size_t length;
  const char *data;
};

static inline int
adftool_dictionary_index_unpack_key (struct adftool_dictionary_index *index,
				     const struct bplus_key *key,
				     size_t *length, char **data)
{
  switch (key->type)
    {
    case BPLUS_KEY_KNOWN:
      return adftool_dictionary_cache_get_a (index->data, key->arg.known,
					     length, data);
    case BPLUS_KEY_UNKNOWN:
      {
	const struct adftool_dictionary_index_key *my_key = key->arg.unknown;
	*length = my_key->length;
	*data = malloc (my_key->length);
	if (*data == NULL)
	  {
	    return 1;
	  }
	memcpy (*data, my_key->data, my_key->length);
	return 0;
      }
    }
  assert (0);
  return 42;
}

static inline int
adftool_dictionary_index_compare (void *context, const struct bplus_key *a,
				  const struct bplus_key *b, int *result)
{
  struct adftool_dictionary_index *self = context;
  size_t a_length, b_length;
  char *a_data = NULL;
  char *b_data = NULL;
  int error_a =
    adftool_dictionary_index_unpack_key (self, a, &a_length, &a_data);
  int error_b =
    adftool_dictionary_index_unpack_key (self, b, &b_length, &b_data);
  if (error_a || error_b)
    {
      if (!error_a)
	{
	  free (a_data);
	}
      if (!error_b)
	{
	  free (b_data);
	}
      return 1;
    }
  size_t min_length = a_length;
  if (min_length > b_length)
    {
      min_length = b_length;
    }
  assert (min_length <= a_length);
  assert (min_length <= b_length);
  assert (min_length == a_length || min_length == b_length);
  if ((*result = memcmp (a_data, b_data, min_length)) == 0)
    {
      /* Longest wins. */
      if (a_length > b_length)
	{
	  *result = 1;
	}
      else if (a_length < b_length)
	{
	  *result = -1;
	}
      else
	{
	  *result = 0;
	}
    }
  free (a_data);
  free (b_data);
  return 0;
}

struct adftool_dictionary_index_iteration_ctx
{
  bool found;
  uint32_t id_found;
};

static inline int
adftool_dictionary_index_found (void *context, size_t n,
				const struct bplus_key *keys,
				const uint32_t * values)
{
  (void) keys;
  struct adftool_dictionary_index_iteration_ctx *ctx = context;
  ctx->found = (ctx->found || n != 0);
  if (n != 0)
    {
      ctx->id_found = values[0];
    }
  if (ctx->found)
    {
      return 1;
    }
  return 0;
}

struct adftool_dictionary_index_decision_ctx
{
  struct adftool_dictionary_index *index;
  uint32_t id_created;
  bool should_insert;
  bool decided_to_abort;
};

static inline int
adftool_dictionary_index_decide (void *context, int present,
				 const struct bplus_key *key, uint32_t * id,
				 int *back)
{
  struct adftool_dictionary_index_decision_ctx *decision = context;
  if (present || !(decision->should_insert))
    {
      decision->decided_to_abort = true;
      return 1;
    }
  decision->decided_to_abort = false;
  size_t length;
  char *data = NULL;
  int unpack_error =
    adftool_dictionary_index_unpack_key (decision->index, key, &length,
					 &data);
  if (unpack_error)
    {
      return 1;
    }
  int insertion_error =
    adftool_dictionary_cache_add (decision->index->data, length, data, id);
  free (data);
  if (insertion_error)
    {
      return 1;
    }
  *back = 0;
  decision->id_created = *id;
  return 0;
}

static int
adftool_dictionary_index_find (struct adftool_dictionary_index *index,
			       size_t length, const char *key,
			       bool insert_if_missing, int *found,
			       uint32_t * id)
{
  struct adftool_dictionary_index_key literal;
  literal.length = length;
  literal.data = key;
  struct adftool_dictionary_index_iteration_ctx iterator;
  iterator.found = false;
  iterator.id_found = ((uint32_t) (-1));
  struct adftool_dictionary_index_decision_ctx decision;
  decision.index = index;
  decision.should_insert = insert_if_missing;
  decision.decided_to_abort = false;
  struct bplus_key unknown;
  unknown.type = BPLUS_KEY_UNKNOWN;
  unknown.arg.unknown = &literal;
  int error = bplus_find (index->tree, bplus_hdf5_fetch, index->handle,
			  adftool_dictionary_index_compare, index,
			  adftool_dictionary_index_found, &iterator,
			  &unknown);
  if (iterator.found)
    {
      *found = 1;
      *id = iterator.id_found;
      return 0;
    }
  else if (error != 0)
    {
      return 1;
    }
  error = bplus_insert (index->tree, bplus_hdf5_fetch, index->handle,
			adftool_dictionary_index_compare, index,
			bplus_hdf5_allocate, index->handle,
			bplus_hdf5_update,
			index->handle, adftool_dictionary_index_decide,
			&decision, &unknown);
  if (decision.decided_to_abort)
    {
      *found = 0;
      *id = (uint32_t) (-1);
      return 0;
    }
  if (error == 0)
    {
      *found = 1;		/* created indeed */
      *id = decision.id_created;
    }
  return error;
}
#endif /* not H_ADFTOOL_DICTIONARY_INDEX_INCLUDED */
