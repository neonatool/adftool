#include <adftool_private.h>

int
adftool_dictionary_get (const struct adftool_file *file, uint32_t id,
			size_t start, size_t max, size_t *length, char *dest)
{
  /* FIXME: A lot of these checks (that strings must be a matrix, with
     13 columns) ought to be performed when loading the file. */
  int error = 0;
  int next_id;
  if (H5Aread (file->dictionary.strings_nextid, H5T_NATIVE_INT, &next_id) < 0)
    {
      error = 1;
      goto wrapup;
    }
  if (next_id < 0 || id >= (uint32_t) next_id)
    {
      error = 1;
      goto wrapup;
    }
  hid_t dataset_space = H5Dget_space (file->dictionary.strings_dataset);
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
  herr_t read_error =
    H5Dread (file->dictionary.strings_dataset, H5T_NATIVE_B8, memory_space,
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
      if (start < bytes_length)
	{
	  bytes_start += start;
	  bytes_length -= start;
	}
      else
	{
	  bytes_length = 0;
	}
      if (bytes_length > max)
	{
	  bytes_length = max;
	}
      hsize_t bytes_memory_length = bytes_length;
      hid_t bytes_memory_space =
	H5Screate_simple (1, &bytes_memory_length, NULL);
      if (bytes_memory_space == H5I_INVALID_HID)
	{
	  error = 1;
	  goto clean_memory_space;
	}
      hid_t bytes_selection_space =
	H5Dget_space (file->dictionary.bytes_dataset);
      if (bytes_selection_space == H5I_INVALID_HID)
	{
	  error = 1;
	  goto clean_bytes_memory_space;
	}
      hsize_t bytes_selection_start[1];
      hsize_t bytes_selection_count[1];
      bytes_selection_start[0] = bytes_start;
      bytes_selection_count[0] = bytes_length;
      if (H5Sselect_hyperslab
	  (bytes_selection_space, H5S_SELECT_AND, bytes_selection_start, NULL,
	   bytes_selection_count, NULL) < 0)
	{
	  error = 1;
	  goto clean_bytes_selection_space;
	}
      herr_t bytes_read_error =
	H5Dread (file->dictionary.bytes_dataset, H5T_NATIVE_B8,
		 bytes_memory_space, bytes_selection_space, H5P_DEFAULT,
		 dest);
      if (bytes_read_error < 0)
	{
	  error = 1;
	  goto clean_bytes_selection_space;
	}
      if (bytes_length < max)
	{
	  dest[bytes_length] = '\0';
	}
    clean_bytes_selection_space:
      H5Sclose (bytes_selection_space);
    clean_bytes_memory_space:
      H5Sclose (bytes_memory_space);
      if (error)
	{
	  goto clean_memory_space;
	}
    }
  else
    {
      for (*length = 0; *length < memory[12]; (*length)++)
	{
	  if (*length >= start && (*length - start) < max)
	    {
	      dest[*length - start] = memory[*length];
	    }
	}
      if (*length >= start && (*length - start) < max)
	{
	  dest[*length - start] = '\0';
	}
      else if (*length < start && max >= 1)
	{
	  dest[0] = '\0';
	}
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

int
adftool_dictionary_lookup (const struct adftool_file *file, size_t length,
			   const char *key, int *found, uint32_t * id)
{
  struct adftool_bplus_key needle;
  struct literal lit;
  lit.length = length;
  lit.data = (char *) key;
  key_set_unknown (&needle, (void *) (&lit));
  size_t n_results;
  int error =
    bplus_lookup (&needle, (struct bplus *) &(file->dictionary.bplus), 0, 1,
		  &n_results, id);
  if (error == 0)
    {
      *found = (n_results != 0);
    }
  return error;
}

static void allocate_string (struct adftool_file *file, size_t length,
			     const char *key, uint32_t * id);

int
adftool_dictionary_insert (struct adftool_file *file, size_t length,
			   const char *key, uint32_t * id)
{
  struct adftool_bplus_key needle;
  struct literal lit;
  lit.length = length;
  lit.data = (char *) key;
  key_set_unknown (&needle, (void *) (&lit));
  size_t n_results;
  int error =
    bplus_lookup (&needle, &(file->dictionary.bplus), 0, 1, &n_results, id);
  if (error)
    {
      goto wrapup;
    }
  if (n_results == 0)
    {
      uint32_t new_id;
      allocate_string (file, length, key, &new_id);
      *id = new_id;
      error = bplus_insert (new_id, new_id, &(file->dictionary.bplus));
      if (error)
	{
	  goto wrapup;
	}
    }
wrapup:
  return error;
}

static void
allocate_string (struct adftool_file *file, size_t length, const char *key,
		 uint32_t * id)
{
  uint8_t memory[13];
  if (length > 12)
    {
      hid_t dataset_space = H5Dget_space (file->dictionary.bytes_dataset);
      if (dataset_space == H5I_INVALID_HID)
	{
	  abort ();
	}
      int rank = H5Sget_simple_extent_ndims (dataset_space);
      if (rank != 1)
	{
	  abort ();
	}
      hsize_t true_dims[1];
      int check_rank =
	H5Sget_simple_extent_dims (dataset_space, true_dims, NULL);
      if (check_rank != 1)
	{
	  abort ();
	}
      long next_id_value;
      herr_t next_id_value_error =
	H5Aread (file->dictionary.bytes_nextid, H5T_NATIVE_LONG,
		 &next_id_value);
      if (next_id_value_error < 0)
	{
	  abort ();
	}
      while (next_id_value + length >= true_dims[0])
	{
	  true_dims[0] *= 2;
	  if (true_dims[0] == 0)
	    {
	      true_dims[0] = 1;
	    }
	  if (H5Dset_extent (file->dictionary.bytes_dataset, true_dims) < 0)
	    {
	      fprintf (stderr, _("Could not extend the dataset.\n"));
	      abort ();
	    }
	}
      uint64_t offset = next_id_value;
      uint32_t bytes_length = length;
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
      hid_t selection_space = H5Screate_simple (1, true_dims, NULL);
      if (selection_space == H5I_INVALID_HID)
	{
	  abort ();
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
	  abort ();
	}
      hsize_t memory_length = length;
      hid_t memory_space = H5Screate_simple (1, &memory_length, NULL);
      if (memory_space == H5I_INVALID_HID)
	{
	  abort ();
	}
      herr_t write_error =
	H5Dwrite (file->dictionary.bytes_dataset, H5T_NATIVE_B8, memory_space,
		  selection_space, H5P_DEFAULT, key);
      if (write_error < 0)
	{
	  abort ();
	}
      H5Sclose (memory_space);
      H5Sclose (selection_space);
      next_id_value += length;
      herr_t update_nextid_error =
	H5Awrite (file->dictionary.bytes_nextid, H5T_NATIVE_INT,
		  &next_id_value);
      if (update_nextid_error)
	{
	  abort ();
	}
    }
  else
    {
      memcpy (memory, key, length);
      memory[12] = length;
    }
  int next_id_value;
  if (H5Aread
      (file->dictionary.strings_nextid, H5T_NATIVE_INT, &next_id_value) < 0)
    {
      abort ();
    }
  hid_t dataset_space = H5Dget_space (file->dictionary.strings_dataset);
  if (dataset_space == H5I_INVALID_HID)
    {
      abort ();
    }
  int rank = H5Sget_simple_extent_ndims (dataset_space);
  if (rank != 2)
    {
      abort ();
    }
  hsize_t true_dims[2];
  int check_rank = H5Sget_simple_extent_dims (dataset_space, true_dims, NULL);
  H5Sclose (dataset_space);
  if (check_rank != 2)
    {
      abort ();
    }
  if (next_id_value >= 0 && (uint32_t) next_id_value == true_dims[0])
    {
      /* We must grow the dataset. */
      true_dims[0] *= 2;
      if (true_dims[0] == 0)
	{
	  true_dims[0] = 1;
	}
      if (H5Dset_extent (file->dictionary.strings_dataset, true_dims) < 0)
	{
	  fprintf (stderr, _("Could not extend the dataset.\n"));
	  abort ();
	}
    }
  *id = next_id_value++;
  hid_t selection_space = H5Screate_simple (2, true_dims, NULL);
  hsize_t start[] = { 0, 0 };
  start[0] = *id;
  hsize_t count[2] = { 1, 13 };
  if (selection_space == H5I_INVALID_HID)
    {
      abort ();
    }
  if (H5Sselect_hyperslab
      (selection_space, H5S_SELECT_AND, start, NULL, count, NULL) < 0)
    {
      abort ();
    }
  hsize_t memory_length = 13;
  hid_t memory_space = H5Screate_simple (1, &memory_length, NULL);
  if (memory_space == H5I_INVALID_HID)
    {
      abort ();
    }
  herr_t write_error =
    H5Dwrite (file->dictionary.strings_dataset, H5T_NATIVE_B8, memory_space,
	      selection_space, H5P_DEFAULT, memory);
  H5Sclose (memory_space);
  H5Sclose (selection_space);
  if (write_error < 0)
    {
      abort ();
    }
  if (H5Awrite
      (file->dictionary.strings_nextid, H5T_NATIVE_INT, &next_id_value) < 0)
    {
      abort ();
    }
}
