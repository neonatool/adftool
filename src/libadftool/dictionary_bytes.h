#ifndef H_ADFTOOL_DICTIONARY_BYTES_INCLUDED
# define H_ADFTOOL_DICTIONARY_BYTES_INCLUDED

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

# define DEALLOC_DICTIONARY_BYTES \
  ATTRIBUTE_DEALLOC (adftool_dictionary_bytes_free, 1)

struct adftool_dictionary_bytes;

static void adftool_dictionary_bytes_free (struct adftool_dictionary_bytes
					   *bytes);

DEALLOC_DICTIONARY_BYTES
  static struct adftool_dictionary_bytes
  *adftool_dictionary_bytes_alloc (void);

static int adftool_dictionary_bytes_setup (struct adftool_dictionary_bytes
					   *bytes, hid_t file);

static int adftool_dictionary_bytes_get (const struct adftool_dictionary_bytes
					 *bytes, uint64_t offset,
					 uint32_t length, char *dst);

static int adftool_dictionary_bytes_append (struct adftool_dictionary_bytes
					    *bytes, uint32_t length,
					    const char *data,
					    uint64_t * offset);

struct adftool_dictionary_bytes
{
  hid_t dataset;
  hid_t nextID;
};

static struct adftool_dictionary_bytes *
adftool_dictionary_bytes_alloc (void)
{
  struct adftool_dictionary_bytes *ret =
    malloc (sizeof (struct adftool_dictionary_bytes));
  if (ret != NULL)
    {
      ret->dataset = H5I_INVALID_HID;
      ret->nextID = H5I_INVALID_HID;
    }
  return ret;
}

static void
adftool_dictionary_bytes_cleanup (struct adftool_dictionary_bytes *bytes)
{
  H5Dclose (bytes->dataset);
  H5Aclose (bytes->nextID);
  bytes->dataset = H5I_INVALID_HID;
  bytes->nextID = H5I_INVALID_HID;
}

static void
adftool_dictionary_bytes_free (struct adftool_dictionary_bytes *bytes)
{
  if (bytes != NULL)
    {
      adftool_dictionary_bytes_cleanup (bytes);
    }
  free (bytes);
}

static int
adftool_dictionary_bytes_setup (struct adftool_dictionary_bytes *bytes,
				hid_t file)
{
  int error = 0;
  bytes->dataset = H5Dopen2 (file, "/dictionary/bytes", H5P_DEFAULT);
  if (bytes->dataset == H5I_INVALID_HID)
    {
      hsize_t minimum_dimensions[] = { 1 };
      hsize_t maximum_dimensions[] = { H5S_UNLIMITED };
      hsize_t chunk_dimensions[] = { 4096 };
      hid_t fspace =
	H5Screate_simple (1, minimum_dimensions, maximum_dimensions);
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
      if (H5Pset_chunk (dataset_creation_properties, 1, chunk_dimensions) < 0)
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
      bytes->dataset =
	H5Dcreate2 (file, "/dictionary/bytes", H5T_NATIVE_B8, fspace,
		    link_creation_properties, dataset_creation_properties,
		    H5P_DEFAULT);
      if (bytes->dataset == H5I_INVALID_HID)
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
  bytes->nextID = H5Aopen (bytes->dataset, "nextID", H5P_DEFAULT);
  if (bytes->nextID == H5I_INVALID_HID)
    {
      hid_t fspace = H5Screate (H5S_SCALAR);
      if (fspace == H5I_INVALID_HID)
	{
	  error = 1;
	  goto cleanup;
	}
      bytes->nextID =
	H5Acreate2 (bytes->dataset, "nextID", H5T_NATIVE_INT,
		    fspace, H5P_DEFAULT, H5P_DEFAULT);
      H5Sclose (fspace);
      if (bytes->nextID == H5I_INVALID_HID)
	{
	  error = 1;
	  goto cleanup;
	}
      int zero = 0;
      if (H5Awrite (bytes->nextID, H5T_NATIVE_INT, &zero) < 0)
	{
	  error = 1;
	  goto cleanup;
	}
    }
cleanup:
  if (error != 0)
    {
      adftool_dictionary_bytes_cleanup (bytes);
    }
  return error;
}

static int
adftool_dictionary_bytes_get (const struct adftool_dictionary_bytes *bytes,
			      uint64_t offset, uint32_t length, char *dst)
{
  int error = 0;
  hid_t file_space = H5Dget_space (bytes->dataset);
  if (file_space == H5I_INVALID_HID)
    {
      error = 1;
      goto cleanup;
    }
  hsize_t file_offset[1] = { offset };
  hsize_t file_count[1] = { length };
  herr_t set_file_space_error =
    H5Sselect_hyperslab (file_space, H5S_SELECT_AND, file_offset, NULL,
			 file_count, NULL);
  if (set_file_space_error < 0)
    {
      error = 1;
      goto cleanup_file_space;
    }
  hsize_t memory_dims[1] = { length };
  hid_t memory_space = H5Screate_simple (1, memory_dims, NULL);
  if (memory_space == H5I_INVALID_HID)
    {
      error = 1;
      goto cleanup_file_space;
    }
  herr_t read_error =
    H5Dread (bytes->dataset, H5T_NATIVE_B8, memory_space, file_space,
	     H5P_DEFAULT, dst);
  if (read_error < 0)
    {
      error = 1;
      goto cleanup_memory_space;
    }
cleanup_memory_space:
  H5Sclose (memory_space);
cleanup_file_space:
  H5Sclose (file_space);
cleanup:
  return error;
}

static int
adftool_dictionary_bytes_append (struct adftool_dictionary_bytes *bytes,
				 uint32_t length, const char *data,
				 uint64_t * offset)
{
  int error = 0;
  hid_t dataset_space = H5Dget_space (bytes->dataset);
  if (dataset_space == H5I_INVALID_HID)
    {
      error = 1;
      goto cleanup;
    }
  int rank = H5Sget_simple_extent_ndims (dataset_space);
  if (rank != 1)
    {
      error = 1;
      goto cleanup_dataset_space;
    }
  hsize_t true_dims[1];
  int check_rank = H5Sget_simple_extent_dims (dataset_space, true_dims, NULL);
  if (check_rank != 1)
    {
      error = 1;
      goto cleanup_dataset_space;
    }
  long next_id_value;
  herr_t next_id_value_error = H5Aread (bytes->nextID, H5T_NATIVE_LONG,
					&next_id_value);
  if (next_id_value_error < 0)
    {
      error = 1;
      goto cleanup_dataset_space;
    }
  assert (next_id_value >= 0);
  while ((size_t) next_id_value + length >= true_dims[0])
    {
      true_dims[0] *= 2;
      if (true_dims[0] == 0)
	{
	  true_dims[0] = 1;
	}
      if (H5Dset_extent (bytes->dataset, true_dims) < 0)
	{
	  error = 1;
	  goto cleanup_dataset_space;
	}
    }
  *offset = next_id_value;
  hid_t selection_space = H5Screate_simple (1, true_dims, NULL);
  if (selection_space == H5I_INVALID_HID)
    {
      error = 1;
      goto cleanup_dataset_space;
    }
  hsize_t start[] = { 0 };
  hsize_t count[] = { 0 };
  start[0] = next_id_value;
  count[0] = length;
  herr_t hyperslab_error =
    H5Sselect_hyperslab (selection_space, H5S_SELECT_AND, start, NULL,
			 count, NULL);
  if (hyperslab_error < 0)
    {
      error = 1;
      goto cleanup_selection_space;
    }
  hsize_t memory_length = length;
  hid_t memory_space = H5Screate_simple (1, &memory_length, NULL);
  if (memory_space == H5I_INVALID_HID)
    {
      error = 1;
      goto cleanup_selection_space;
    }
  herr_t write_error = H5Dwrite (bytes->dataset, H5T_NATIVE_B8, memory_space,
				 selection_space, H5P_DEFAULT, data);
  if (write_error < 0)
    {
      error = 1;
      goto cleanup_memory_space;
    }
  next_id_value += length;
  herr_t update_nextid_error =
    H5Awrite (bytes->nextID, H5T_NATIVE_INT, &next_id_value);
  if (update_nextid_error)
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

#endif /* not H_ADFTOOL_DICTIONARY_BYTES_INCLUDED */
