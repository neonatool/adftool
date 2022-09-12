#include <adftool_private.h>
#include <adftool_bplus.h>

#include <float.h>

static void compute_encoding (size_t n, size_t p, size_t i,
			      const double *data, double *offset,
			      double *scale);

static struct adftool_statement *new_channel_statement (size_t i);

static int set_channel_identifier (struct adftool_file *file,
				   size_t channel_index,
				   const struct adftool_term *identifier);

int
adftool_eeg_set_data (struct adftool_file *file, size_t n_points,
		      size_t n_channels, const double *data)
{
  int error = 0;
  if (file->eeg_dataset != H5I_INVALID_HID)
    {
      H5Dclose (file->eeg_dataset);
      file->eeg_dataset = H5I_INVALID_HID;
    }
  H5Ldelete (file->hdf5_file, "/eeg-data", H5P_DEFAULT);
  hsize_t dimensions[2];
  dimensions[0] = n_points;
  dimensions[1] = n_channels;
  hid_t fspace = H5Screate_simple (2, dimensions, dimensions);
  if (fspace == H5I_INVALID_HID)
    {
      error = 1;
      goto wrapup;
    }
  file->eeg_dataset =
    H5Dcreate2 (file->hdf5_file, "/eeg-data", H5T_NATIVE_B16, fspace,
		H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  if (file->eeg_dataset == H5I_INVALID_HID)
    {
      error = 1;
      goto clean_fspace;
    }
  for (size_t i = 0; i < n_channels; i++)
    {
      double offset;
      double scale;
      compute_encoding (n_points, n_channels, i, data, &offset, &scale);
      struct adftool_statement *new_channel = new_channel_statement (i);
      if (new_channel == NULL)
	{
	  abort ();
	}
      if (adftool_insert (file, new_channel) != 0)
	{
	  error = 1;
	  goto clean_new_channel;
	}
      const struct adftool_term *identifier;
      adftool_statement_get (new_channel, NULL, NULL,
			     (struct adftool_term **) &identifier, NULL,
			     NULL);
      assert (identifier != NULL);
      if (set_channel_identifier (file, i, identifier) != 0)
	{
	  error = 1;
	  goto clean_new_channel;
	}
      if (adftool_set_channel_decoder (file, identifier, scale, offset) != 0)
	{
	  error = 1;
	  goto clean_new_channel;
	}
      hid_t selection_space = H5Screate_simple (2, dimensions, NULL);
      if (selection_space == H5I_INVALID_HID)
	{
	  error = 1;
	  goto clean_fspace;
	}
      hsize_t start[] = { 0, 0 };
      hsize_t count[] = { 0, 1 };
      start[1] = i;
      count[0] = n_points;
      herr_t selection_error =
	H5Sselect_hyperslab (selection_space, H5S_SELECT_AND, start, NULL,
			     count, NULL);
      if (selection_error)
	{
	  error = 1;
	  goto clean_selection_space;
	}
      hsize_t mem_length = n_points;
      hid_t memspace = H5Screate_simple (1, &mem_length, &mem_length);
      if (memspace == H5I_INVALID_HID)
	{
	  error = 1;
	  goto clean_selection_space;
	}
      uint16_t *encoded = malloc (n_points * sizeof (uint16_t));
      if (encoded == NULL)
	{
	  error = 1;
	  goto clean_memory_space;
	}
      for (size_t j = 0; j < n_points; j++)
	{
	  const double raw = data[j * n_channels + i];
	  const double encoded_point = (raw - offset) / scale;
	  const uint16_t rounded = (uint16_t) (encoded_point + 0.5);
	  encoded[j] = rounded;
	}
      herr_t write_error =
	H5Dwrite (file->eeg_dataset, H5T_NATIVE_B16, memspace,
		  selection_space, H5P_DEFAULT, encoded);
      if (write_error < 0)
	{
	  error = 1;
	  goto clean_encoded;
	}
    clean_encoded:
      free (encoded);
    clean_memory_space:
      H5Sclose (memspace);
    clean_selection_space:
      H5Sclose (selection_space);
    clean_new_channel:
      adftool_statement_free (new_channel);
      if (error)
	{
	  goto clean_fspace;
	}
    }
clean_fspace:
  H5Sclose (fspace);
wrapup:
  return error;
}

int
adftool_eeg_get_data (const struct adftool_file *file, size_t time_start,
		      size_t time_length, size_t *time_max,
		      size_t channel_start, size_t channel_length,
		      size_t *channel_max, double *data)
{
  int error = 0;
  if (file->eeg_dataset == H5I_INVALID_HID)
    {
      error = 1;
      goto wrapup;
    }
  hid_t dataspace = H5Dget_space (file->eeg_dataset);
  if (dataspace == H5I_INVALID_HID)
    {
      error = 1;
      goto wrapup;
    }
  hsize_t dimensions[2];
  if (H5Sget_simple_extent_ndims (dataspace) != 2)
    {
      error = 1;
      goto clean_dataspace;
    }
  if (H5Sget_simple_extent_dims (dataspace, dimensions, NULL) != 2)
    {
      error = 1;
      goto clean_dataspace;
    }
  *time_max = dimensions[0];
  *channel_max = dimensions[1];
  for (size_t j = 0; j < dimensions[1]; j++)
    {
      struct adftool_term *identifier = adftool_term_alloc ();
      if (identifier == NULL)
	{
	  error = 1;
	  goto clean_dataspace;
	}
      if (adftool_find_channel_identifier (file, j, identifier) != 0)
	{
	  error = 1;
	  goto clean_identifier;
	}
      double scale, offset;
      if (adftool_get_channel_decoder (file, identifier, &scale, &offset) !=
	  0)
	{
	  error = 1;
	  goto clean_identifier;
	}

      hid_t selection_space = H5Screate_simple (2, dimensions, NULL);
      if (selection_space == H5I_INVALID_HID)
	{
	  error = 1;
	  goto clean_identifier;
	}
      hsize_t start[] = { 0, 0 };
      hsize_t count[] = { 0, 1 };
      start[1] = j;
      count[0] = dimensions[0];
      herr_t selection_error =
	H5Sselect_hyperslab (selection_space, H5S_SELECT_AND, start, NULL,
			     count, NULL);
      if (selection_error)
	{
	  error = 1;
	  goto clean_selection_space;
	}
      hsize_t mem_length = dimensions[0];
      hid_t memspace = H5Screate_simple (1, &mem_length, &mem_length);
      if (memspace == H5I_INVALID_HID)
	{
	  error = 1;
	  goto clean_selection_space;
	}
      uint16_t *encoded = malloc (dimensions[0] * sizeof (uint16_t));
      if (encoded == NULL)
	{
	  error = 1;
	  goto clean_memory_space;
	}
      herr_t read_error =
	H5Dread (file->eeg_dataset, H5T_NATIVE_B16, memspace, selection_space,
		 H5P_DEFAULT, encoded);
      if (read_error < 0)
	{
	  error = 1;
	  goto clean_encoded;
	}
      for (size_t i = 0; i < dimensions[0]; i++)
	{
	  const double raw = encoded[i];
	  const double scaled = raw * scale + offset;
	  if (i >= time_start && i - time_start < time_length
	      && j >= channel_start && j - channel_start < channel_length)
	    {
	      data[i * channel_length + j] = scaled;
	    }
	}
    clean_encoded:
      free (encoded);
    clean_memory_space:
      H5Sclose (memspace);
    clean_selection_space:
      H5Sclose (selection_space);
    clean_identifier:
      adftool_term_free (identifier);
      if (error)
	{
	  goto clean_dataspace;
	}
    }
clean_dataspace:
  H5Sclose (dataspace);
wrapup:
  return error;
}

static void
compute_encoding (size_t n, size_t p, size_t j, const double *data,
		  double *offset, double *scale)
{
  double mini = DBL_MAX;
  double maxi = -DBL_MAX;
  for (size_t i = 0; i < n; i++)
    {
      const double value = data[i * p + j];
      if (value < mini)
	{
	  mini = value;
	}
      if (value > maxi)
	{
	  maxi = value;
	}
    }
  const double input_mini = 0;
  const double input_maxi = 65535;
  /* Now find offset and scale such that: */
  /* input_maxi * scale + offset = maxi */
  /* input_mini * scale + offset = mini */

  /* Take the difference: */
  /* (input_maxi - input_mini) * scale = maxi - mini */
  *scale = (maxi - mini) / (input_maxi - input_mini);

  /* Replace: */
  *offset = maxi - input_maxi * (*scale);
}

static struct adftool_statement *
new_channel_statement (size_t i)
{
  struct adftool_statement *statement = adftool_statement_alloc ();
  if (statement == NULL)
    {
      goto wrapup;
    }
  struct adftool_term *subject = adftool_term_alloc ();
  if (subject == NULL)
    {
      adftool_statement_free (statement);
      statement = NULL;
      goto wrapup;
    }
  struct adftool_term *predicate = adftool_term_alloc ();
  if (predicate == NULL)
    {
      adftool_statement_free (statement);
      statement = NULL;
      goto cleanup_subject;
    }
  struct adftool_term *object = adftool_term_alloc ();
  if (object == NULL)
    {
      adftool_statement_free (statement);
      statement = NULL;
      goto cleanup_predicate;
    }
  /* <> <https://localhost/lytonepal#has-channel> <#channel-%lu> . */
  char label[64];
  sprintf (label, "#channel-%lu", i);
  static const char *pred = "https://localhost/lytonepal#has-channel";
  /* 64 chars is enough I guess. There are up to 64 digits in base 2,
     so around a third of that in base 10. */
  adftool_term_set_named (subject, "");
  adftool_term_set_named (predicate, pred);
  adftool_term_set_named (object, label);
  struct adftool_term *graph = NULL;
  uint64_t deletion_date = ((uint64_t) (-1));
  adftool_statement_set (statement, &subject, &predicate, &object, &graph,
			 &deletion_date);
  adftool_term_free (object);
cleanup_predicate:
  adftool_term_free (predicate);
cleanup_subject:
  adftool_term_free (subject);
wrapup:
  return statement;
}

static int
set_channel_identifier (struct adftool_file *file,
			size_t channel_index,
			const struct adftool_term *identifier)
{
  int error = 0;
  struct adftool_term *object = adftool_term_alloc ();
  mpz_t i;
  mpz_init_set_ui (i, channel_index);
  if (object == NULL)
    {
      abort ();
    }
  adftool_term_set_integer (object, i);
  mpz_clear (i);
  struct adftool_term predicate = {
    .type = TERM_NAMED,
    .str1 = "https://localhost/lytonepal#column-number",
    .str2 = NULL
  };
  struct adftool_statement pattern = {
    .subject = NULL,
    .predicate = &predicate,
    .object = object,
    .graph = NULL,
    .deletion_date = ((uint64_t) (-1))
  };
  if (adftool_delete (file, &pattern, time (NULL) * 1000) != 0)
    {
      error = 1;
      goto cleanup;
    }
  pattern.subject = (struct adftool_term *) identifier;
  if (adftool_insert (file, &pattern) != 0)
    {
      error = 1;
      goto cleanup;
    }
cleanup:
  adftool_term_free (object);
  return error;
}
