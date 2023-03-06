#ifndef H_ADFTOOL_DICTIONARY_STRINGS_INCLUDED
# define H_ADFTOOL_DICTIONARY_STRINGS_INCLUDED

# include <adftool.h>
# include <bplus.h>
# include <hdf5.h>

# include "dictionary_bytes.h"

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

struct adftool_dictionary_strings;

static struct adftool_dictionary_strings
  *adftool_dictionary_strings_alloc (void);
static void
adftool_dictionary_strings_free (struct adftool_dictionary_strings *strings);

static int
adftool_dictionary_strings_setup (struct adftool_dictionary_strings *strings,
				  hid_t file);

static int
adftool_dictionary_strings_get_a (const struct adftool_dictionary_strings
				  *strings, uint32_t id, size_t *length,
				  char **data);

static int
adftool_dictionary_strings_add (struct adftool_dictionary_strings *strings,
				uint32_t length, const char *data,
				uint32_t * id);

struct adftool_dictionary_strings
{
  hid_t dataset;
  hid_t nextID;
  struct adftool_dictionary_bytes *bytes;
};

static struct adftool_dictionary_strings *
adftool_dictionary_strings_alloc (void)
{
  struct adftool_dictionary_strings *ret =
    malloc (sizeof (struct adftool_dictionary_strings));
  if (ret != NULL)
    {
      ret->dataset = H5I_INVALID_HID;
      ret->nextID = H5I_INVALID_HID;
      ret->bytes = adftool_dictionary_bytes_alloc ();
      if (ret->bytes == NULL)
	{
	  free (ret);
	  ret = NULL;
	}
    }
  return ret;
}

static void
adftool_dictionary_strings_cleanup (struct adftool_dictionary_strings
				    *strings)
{
  H5Dclose (strings->dataset);
  H5Aclose (strings->nextID);
  strings->dataset = H5I_INVALID_HID;
  strings->nextID = H5I_INVALID_HID;
  adftool_dictionary_bytes_cleanup (strings->bytes);
}

static void
adftool_dictionary_strings_free (struct adftool_dictionary_strings *strings)
{
  if (strings != NULL)
    {
      adftool_dictionary_strings_cleanup (strings);
      adftool_dictionary_bytes_free (strings->bytes);
    }
  free (strings);
}

static int
adftool_dictionary_strings_setup (struct adftool_dictionary_strings *strings,
				  hid_t file)
{
  int error = 0;
  strings->dataset = H5Dopen2 (file, "/dictionary/strings", H5P_DEFAULT);
  if (strings->dataset == H5I_INVALID_HID)
    {
      hsize_t minimum_dimensions[] = { 1, 13 };
      hsize_t maximum_dimensions[] = { H5S_UNLIMITED, 13 };
      hsize_t chunk_dimensions[] = { 1, 13 };
      hid_t fspace =
	H5Screate_simple (2, minimum_dimensions, maximum_dimensions);
      if (fspace == H5I_INVALID_HID)
	{
	  error = 1;
	  goto cleanup;
	}
      hid_t dataset_creation_properties = H5Pcreate (H5P_DATASET_CREATE);
      if (dataset_creation_properties == H5I_INVALID_HID)
	{
	  error = 1;
	  goto cleanup_fspace;
	}
      if (H5Pset_chunk (dataset_creation_properties, 2, chunk_dimensions) < 0)
	{
	  error = 1;
	  goto cleanup_dcpl;
	}
      hid_t link_creation_properties = H5Pcreate (H5P_LINK_CREATE);
      if (link_creation_properties == H5I_INVALID_HID)
	{
	  error = 1;
	  goto cleanup_dcpl;
	}
      if (H5Pset_create_intermediate_group (link_creation_properties, 1) < 0)
	{
	  error = 1;
	  goto cleanup_lcpl;
	}
      strings->dataset =
	H5Dcreate2 (file, "/dictionary/strings", H5T_NATIVE_B8, fspace,
		    link_creation_properties, dataset_creation_properties,
		    H5P_DEFAULT);
      if (strings->dataset == H5I_INVALID_HID)
	{
	  error = 1;
	  goto cleanup_lcpl;
	}
    cleanup_lcpl:
      H5Pclose (link_creation_properties);
    cleanup_dcpl:
      H5Pclose (dataset_creation_properties);
    cleanup_fspace:
      H5Sclose (fspace);
      if (error != 0)
	{
	  goto cleanup;
	}
    }
  strings->nextID = H5Aopen (strings->dataset, "nextID", H5P_DEFAULT);
  if (strings->nextID == H5I_INVALID_HID)
    {
      hid_t fspace = H5Screate (H5S_SCALAR);
      if (fspace == H5I_INVALID_HID)
	{
	  error = 1;
	  goto cleanup;
	}
      strings->nextID =
	H5Acreate2 (strings->dataset, "nextID", H5T_NATIVE_INT,
		    fspace, H5P_DEFAULT, H5P_DEFAULT);
      H5Sclose (fspace);
      if (strings->nextID == H5I_INVALID_HID)
	{
	  error = 1;
	  goto cleanup;
	}
      int zero = 0;
      if (H5Awrite (strings->nextID, H5T_NATIVE_INT, &zero) < 0)
	{
	  error = 1;
	  goto cleanup;
	}
    }
  int bytes_error = adftool_dictionary_bytes_setup (strings->bytes, file);
  if (bytes_error)
    {
      error = 1;
      goto cleanup;
    }
cleanup:
  if (error != 0)
    {
      adftool_dictionary_strings_cleanup (strings);
    }
  return error;
}

static int
adftool_dictionary_strings_get_a (const struct adftool_dictionary_strings
				  *strings, uint32_t id, size_t *length,
				  char **data)
{
  int error = 0;
  int next_id;
  if (H5Aread (strings->nextID, H5T_NATIVE_INT, &next_id) < 0)
    {
      error = 1;
      goto wrapup;
    }
  if (next_id < 0 || id >= (uint32_t) next_id)
    {
      error = 1;
      goto wrapup;
    }
  hid_t dataset_space = H5Dget_space (strings->dataset);
  if (dataset_space == H5I_INVALID_HID)
    {
      error = 1;
      goto wrapup;
    }
  int rank = H5Sget_simple_extent_ndims (dataset_space);
  if (rank != 2)
    {
      error = 1;
      goto clean_dataset_space;
    }
  hsize_t true_dims[2];
  int check_rank = H5Sget_simple_extent_dims (dataset_space, true_dims, NULL);
  if (check_rank < 0 || check_rank != rank)
    {
      error = 1;
      goto clean_dataset_space;
    }
  if (true_dims[1] != 13)
    {
      error = 1;
      goto clean_dataset_space;
    }
  hsize_t selection_start[2] = { 0, 0 };
  hsize_t selection_count[2] = { 1, 13 };
  selection_start[0] = id;
  hid_t selection_space = H5Screate_simple (2, true_dims, NULL);
  if (selection_space == H5I_INVALID_HID)
    {
      error = 1;
      goto clean_dataset_space;
    }
  if (H5Sselect_hyperslab
      (selection_space, H5S_SELECT_AND, selection_start, NULL,
       selection_count, NULL) < 0)
    {
      error = 1;
      goto clean_selection_space;
    }
  hsize_t memory_length = 13;
  uint8_t memory[13];
  hid_t memory_space = H5Screate_simple (1, &memory_length, NULL);
  if (memory_space == H5I_INVALID_HID)
    {
      error = 1;
      goto clean_selection_space;
    }
  herr_t read_error = H5Dread (strings->dataset, H5T_NATIVE_B8, memory_space,
			       selection_space, H5P_DEFAULT, memory);
  if (read_error < 0)
    {
      error = 1;
      goto clean_memory_space;
    }
  uint64_t bytes_start = 0;
  uint32_t bytes_length = 0;
  for (size_t i = 0; i < 8; i++)
    {
      bytes_start *= 256;
      bytes_start += memory[i];
    }
  for (size_t i = 8; i < 12; i++)
    {
      bytes_length *= 256;
      bytes_length += memory[i];
    }
  if (memory[12] == 0 && bytes_length != 0)
    {
      /* This is a long string. */
      *length = bytes_length;
      *data = malloc (bytes_length + 1);
      if (*data == NULL)
	{
	  error = 1;
	  goto clean_memory_space;
	}
      int bytes_error =
	adftool_dictionary_bytes_get (strings->bytes, bytes_start,
				      bytes_length, *data);
      if (bytes_error)
	{
	  error = 1;
	  free (*data);
	  /* GCC static analyzer thinks *data leaks here, which is
	     obviously wrong, because we called free (*data), and this
	     is the same value as what was mallocated. */
	  *data = NULL;
	  goto clean_memory_space;
	}
      (*data)[bytes_length] = '\0';
    }
  else
    {
      *length = memory[12];
      *data = NULL;
      if (*length <= 12)
	{
	  *data = malloc (*length + 1);
	}
      else
	{
	  /* Failure: The thirteenth column of the strings dataset
	     must contain either 0 or a number at most equal to 12. */
	}
      if (*data == NULL)
	{
	  error = 1;
	  goto clean_memory_space;
	}
      memcpy (*data, memory, *length);
      (*data)[*length] = '\0';
    }
clean_memory_space:
  H5Sclose (memory_space);
clean_selection_space:
  H5Sclose (selection_space);
clean_dataset_space:
  H5Sclose (dataset_space);
wrapup:
  return error;
}

static int
adftool_dictionary_strings_add (struct adftool_dictionary_strings *strings,
				uint32_t length, const char *data,
				uint32_t * id)
{
  int error = 0;
  uint8_t memory[13];
  if (length > 12)
    {
      uint64_t offset;
      uint32_t bytes_length = length;
      int bytes_error =
	adftool_dictionary_bytes_append (strings->bytes, length, data,
					 &offset);
      if (bytes_error)
	{
	  error = 1;
	  goto cleanup;
	}
      for (size_t i = 8; i-- > 0;)
	{
	  memory[i] = (offset % 256);
	  offset /= 256;
	}
      for (size_t i = 12; i-- > 8;)
	{
	  memory[i] = (bytes_length % 256);
	  bytes_length /= 256;
	}
      memory[12] = 0;
    }
  else
    {
      memcpy (memory, data, length);
      memory[12] = length;
    }
  int next_id_value;
  if (H5Aread (strings->nextID, H5T_NATIVE_INT, &next_id_value) < 0)
    {
      error = 1;
      goto cleanup;
    }
  hid_t dataset_space = H5Dget_space (strings->dataset);
  if (dataset_space == H5I_INVALID_HID)
    {
      error = 1;
      goto cleanup;
    }
  int rank = H5Sget_simple_extent_ndims (dataset_space);
  if (rank != 2)
    {
      error = 1;
      goto cleanup_dataset_space;
    }
  hsize_t true_dims[2];
  int check_rank = H5Sget_simple_extent_dims (dataset_space, true_dims, NULL);
  if (check_rank != 2)
    {
      error = 1;
      goto cleanup_dataset_space;
    }
  if (next_id_value >= 0 && (uint32_t) next_id_value == true_dims[0])
    {
      /* We must grow the dataset. */
      true_dims[0] *= 2;
      if (true_dims[0] == 0)
	{
	  true_dims[0] = 1;
	}
      if (H5Dset_extent (strings->dataset, true_dims) < 0)
	{
	  error = 1;
	  goto cleanup_dataset_space;
	}
    }
  *id = next_id_value++;
  hid_t selection_space = H5Screate_simple (2, true_dims, NULL);
  hsize_t start[] = { 0, 0 };
  start[0] = *id;
  hsize_t count[2] = { 1, 13 };
  if (selection_space == H5I_INVALID_HID)
    {
      error = 1;
      goto cleanup_dataset_space;
    }
  if (H5Sselect_hyperslab
      (selection_space, H5S_SELECT_AND, start, NULL, count, NULL) < 0)
    {
      error = 1;
      goto cleanup_selection_space;
    }
  hsize_t memory_length = 13;
  hid_t memory_space = H5Screate_simple (1, &memory_length, NULL);
  if (memory_space == H5I_INVALID_HID)
    {
      error = 1;
      goto cleanup_selection_space;
    }
  herr_t write_error =
    H5Dwrite (strings->dataset, H5T_NATIVE_B8, memory_space,
	      selection_space, H5P_DEFAULT, memory);
  if (write_error < 0)
    {
      error = 1;
      goto cleanup_memory_space;
    }
  if (H5Awrite (strings->nextID, H5T_NATIVE_INT, &next_id_value) < 0)
    {
      error = 1;
      goto cleanup_memory_space;
    }
cleanup_memory_space:
  H5Sclose (memory_space);
cleanup_selection_space:
  H5Sclose (selection_space);
cleanup_dataset_space:
  H5Sclose (dataset_space);
cleanup:
  return error;
}

#endif /* not H_ADFTOOL_DICTIONARY_STRINGS_INCLUDED */
