#include <config.h>
#include <attribute.h>
#include <adftool.h>

#define STREQ(s1, s2) (strcmp ((s1), (s2)) == 0)
#define STRNEQ(s1, s2) (strcmp ((s1), (s2)) != 0)

#include "file.h"

#include <hdf5.h>
#include <unistd.h>

#define DEFAULT_ORDER 256

#if defined _WIN32
# define OPEN_BINARY_SUFFIX "b"
#else
# define OPEN_BINARY_SUFFIX ""
#endif

#define DEFAULT_DICTIONARY_CACHE_ENTRIES 8191
#define DEFAULT_DICTIONARY_CACHE_ENTRY_LENGTH 512

struct adftool_file *
adftool_file_open (const char *filename, int write)
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

struct adftool_file *
adftool_file_open_data (size_t nbytes, const void *bytes)
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
  if (nbytes != 0)
    {
      n_written = fwrite (bytes, 1, nbytes, f);
    }
  if (n_written != nbytes)
    {
      goto close_file;
    }
  if (fflush (f) != 0)
    {
      goto close_file;
    }
  ret = adftool_file_open (filename, 1);
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

void
adftool_file_close (struct adftool_file *file)
{
  adftool_file_free (file);
}

size_t
adftool_file_get_data (struct adftool_file *file, size_t start, size_t max,
		       void *bytes)
{
  if (H5Fflush (file->hdf5_handle, H5F_SCOPE_GLOBAL) < 0)
    {
      return 0;
    }
  ssize_t required = H5Fget_file_image (file->hdf5_handle, NULL, 0);
  if (required < 0)
    {
      return 0;
    }
  if (max == 0)
    {
      return required;
    }
  char *all_bytes = malloc (required);
  if (all_bytes == NULL)
    {
      return 0;
    }
  ssize_t check = H5Fget_file_image (file->hdf5_handle, all_bytes, required);
  const size_t file_length = check;
  if (check < 0)
    {
      /* Error the second time */
      free (all_bytes);
      return 0;
    }
  if (check > required)
    {
      /* Retry */
      free (all_bytes);
      return adftool_file_get_data (file, start, max, bytes);
    }
  if (start > file_length)
    {
      /* Nothing to set. */
      free (all_bytes);
      return file_length;
    }
  if (start + max > file_length)
    {
      /* Do not fill all of bytes, because the caller is too
         generous. */
      assert (start <= file_length);
      max = file_length - start;
    }
  if (max != 0)
    {
      /* See earlier in the function. It is forbidden to copy 0 bytes
         with memcpy, so we protect it. However, we already returned
         early if max == 0, because we need no copy at all. */
      memcpy (bytes, all_bytes + start, max);
    }
  free (all_bytes);
  return file_length;
}

int
adftool_dictionary_get (struct adftool_file *file, uint32_t id, size_t start,
			size_t max, size_t *length, char *dest)
{
  size_t true_length;
  char *true_data;
  int error =
    adftool_dictionary_cache_get_a (file->dictionary->data, id, &true_length,
				    &true_data);
  if (error)
    {
      return 1;
    }
  for (size_t i = 0; i < true_length; i++)
    {
      if (i >= start && i - start < max)
	{
	  dest[i - start] = true_data[i];
	}
    }
  if (true_length >= start && true_length - start < max)
    {
      dest[true_length - start] = '\0';
    }
  *length = true_length;
  free (true_data);
  return 0;
}

int
adftool_dictionary_lookup (struct adftool_file *file, size_t length,
			   const char *key, int *found, uint32_t * id)
{
  return adftool_dictionary_index_find (file->dictionary, length, key, false,
					found, id);
}

int
adftool_dictionary_insert (struct adftool_file *file, size_t length,
			   const char *key, uint32_t * id)
{
  int found;
  int error =
    adftool_dictionary_index_find (file->dictionary, length, key, true,
				   &found, id);
  if (error == 0)
    {
      assert (found);
    }
  return error;
}
