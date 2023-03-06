#include <config.h>
#include <adftool.h>

#define STREQ(s1, s2) (strcmp ((s1), (s2)) == 0)
#define STRNEQ(s1, s2) (strcmp ((s1), (s2)) != 0)

#include "file.h"
#include "channel_decoder.h"

#include <float.h>

#include <hdf5.h>

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
  hid_t eeg_dataset = H5I_INVALID_HID;
  H5Ldelete (file->hdf5_handle, "/eeg-data", H5P_DEFAULT);
  hsize_t dimensions[2];
  dimensions[0] = n_points;
  dimensions[1] = n_channels;
  hid_t fspace = H5Screate_simple (2, dimensions, dimensions);
  if (fspace == H5I_INVALID_HID)
    {
      error = 1;
      goto wrapup;
    }
  eeg_dataset =
    H5Dcreate2 (file->hdf5_handle, "/eeg-data", H5T_NATIVE_B16, fspace,
		H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  if (eeg_dataset == H5I_INVALID_HID)
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
      if (adftool_file_insert (file, new_channel) != 0)
	{
	  error = 1;
	  goto clean_new_channel;
	}
      const struct adftool_term *identifier;
      statement_get (new_channel, NULL, NULL,
		     (struct adftool_term **) &identifier, NULL, NULL);
      assert (identifier != NULL);
      if (set_channel_identifier (file, i, identifier) != 0)
	{
	  error = 1;
	  goto clean_new_channel;
	}
      if (channel_decoder_set (file, identifier, scale, offset) != 0)
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
      herr_t write_error = H5Dwrite (eeg_dataset, H5T_NATIVE_B16, memspace,
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
      statement_free (new_channel);
      if (error)
	{
	  goto clean_fspace;
	}
    }
clean_fspace:
  H5Sclose (fspace);
wrapup:
  H5Dclose (eeg_dataset);
  return error;
}

int
adftool_eeg_get_data (struct adftool_file *file, size_t time_start,
		      size_t time_length, size_t *time_max,
		      size_t channel_start, size_t channel_length,
		      size_t *channel_max, double *data)
{
  int error = 0;
  hid_t eeg_dataset = H5Dopen2 (file->hdf5_handle, "/eeg-data", H5P_DEFAULT);
  if (eeg_dataset == H5I_INVALID_HID)
    {
      error = 1;
      goto wrapup;
    }
  hid_t dataspace = H5Dget_space (eeg_dataset);
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
  /* We are filling out rows of data. Data is a row-wise matrix, with
     channel_length columns. */
  const size_t output_row_length = channel_length;
  /* Now we can restrict channel_length so as to make sure each
     requested channel is in the file. If channel_length is too large
     (channel_start + channel_length is out of bounds), then the last
     columns of the output will not be touched. */
  if (time_start >= dimensions[0])
    {
      time_start = 0;
      time_length = 0;
    }
  if (channel_start >= dimensions[1])
    {
      channel_start = 0;
      channel_length = 0;
    }
  if (time_start + time_length > dimensions[0])
    {
      time_length = dimensions[0] - time_start;
    }
  if (channel_length >= dimensions[1])
    {
      channel_length = dimensions[1] - channel_start;
    }
  for (size_t j = channel_start; j - channel_start < channel_length; j++)
    {
      struct adftool_term *identifier = term_alloc ();
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
      if (channel_decoder_get (file, identifier, &scale, &offset) != 0)
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
      /* We want to read data starting at channel j (1 channel), time
         starting at time_start (time_length points) */
      hsize_t start[] = { 0, 0 };
      hsize_t count[] = { 0, 1 };
      start[0] = time_start;
      start[1] = j;
      count[0] = time_length;
      herr_t selection_error =
	H5Sselect_hyperslab (selection_space, H5S_SELECT_AND, start, NULL,
			     count, NULL);
      if (selection_error)
	{
	  error = 1;
	  goto clean_selection_space;
	}
      /* We want to store the result in an array of length
         time_length. */
      hsize_t mem_length = time_length;
      hid_t memspace = H5Screate_simple (1, &mem_length, &mem_length);
      if (memspace == H5I_INVALID_HID)
	{
	  error = 1;
	  goto clean_selection_space;
	}
      uint16_t *encoded = malloc (time_length * sizeof (uint16_t));
      if (encoded == NULL)
	{
	  error = 1;
	  goto clean_memory_space;
	}
      herr_t read_error =
	H5Dread (eeg_dataset, H5T_NATIVE_B16, memspace, selection_space,
		 H5P_DEFAULT, encoded);
      if (read_error < 0)
	{
	  error = 1;
	  goto clean_encoded;
	}
      for (size_t i = time_start; i - time_start < time_length; i++)
	{
	  const size_t time_index = i - time_start;
	  const size_t channel_index = j - channel_start;
	  const size_t index = time_index * output_row_length + channel_index;
	  const double raw = encoded[i - time_start];
	  const double scaled = raw * scale + offset;
	  data[index] = scaled;
	}
    clean_encoded:
      free (encoded);
    clean_memory_space:
      H5Sclose (memspace);
    clean_selection_space:
      H5Sclose (selection_space);
    clean_identifier:
      term_free (identifier);
      if (error)
	{
	  goto clean_dataspace;
	}
    }
clean_dataspace:
  H5Sclose (dataspace);
wrapup:
  H5Dclose (eeg_dataset);
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
  struct adftool_statement *statement = statement_alloc ();
  if (statement == NULL)
    {
      goto wrapup;
    }
  struct adftool_term *subject = term_alloc ();
  if (subject == NULL)
    {
      statement_free (statement);
      statement = NULL;
      goto wrapup;
    }
  struct adftool_term *predicate = term_alloc ();
  if (predicate == NULL)
    {
      statement_free (statement);
      statement = NULL;
      goto cleanup_subject;
    }
  struct adftool_term *object = term_alloc ();
  if (object == NULL)
    {
      statement_free (statement);
      statement = NULL;
      goto cleanup_predicate;
    }
  /* <> <https://localhost/lytonepal#has-channel> <#channel-%lu> . */
  char label[64];
  sprintf (label, "#channel-%lu", i);
  static const char *pred = "https://localhost/lytonepal#has-channel";
  /* 64 chars is enough I guess. There are up to 64 digits in base 2,
     so around a third of that in base 10. */
  term_set_named (subject, "");
  term_set_named (predicate, pred);
  term_set_named (object, label);
  struct adftool_term *graph = NULL;
  uint64_t deletion_date = ((uint64_t) (-1));
  statement_set (statement, &subject, &predicate, &object, &graph,
		 &deletion_date);
  term_free (object);
cleanup_predicate:
  term_free (predicate);
cleanup_subject:
  term_free (subject);
wrapup:
  return statement;
}

static int
set_channel_identifier (struct adftool_file *file,
			size_t channel_index,
			const struct adftool_term *identifier)
{
  int error = 0;
  struct adftool_term *object = term_alloc ();
  mpz_t i;
  mpz_init_set_ui (i, channel_index);
  if (object == NULL)
    {
      abort ();
    }
  term_set_mpz (object, i);
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
  if (adftool_file_delete (file, &pattern, time (NULL) * 1000) != 0)
    {
      error = 1;
      goto cleanup;
    }
  pattern.subject = (struct adftool_term *) identifier;
  if (adftool_file_insert (file, &pattern) != 0)
    {
      error = 1;
      goto cleanup;
    }
cleanup:
  term_free (object);
  return error;
}
