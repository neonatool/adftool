#include <config.h>
#include <attribute.h>
#include <adftool.h>
#include <unistd.h>

#define STREQ(s1, s2) (strcmp ((s1), (s2)) == 0)
#define STRNEQ(s1, s2) (strcmp ((s1), (s2)) != 0)

#include "file.h"
#include "generate.h"

#include <hdf5.h>

struct adftool_file *
adftool_file_open (const char *filename, int write)
{
  return file_open (filename, write);
}

struct adftool_file *
adftool_file_open_data (size_t nbytes, const void *bytes)
{
  return file_open_data (nbytes, bytes);
}

struct adftool_file *
adftool_file_open_generated (void)
{
  return file_open_generated ();
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
