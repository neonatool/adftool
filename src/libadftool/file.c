#include <adftool_private.h>
#include <adftool_bplus.h>

#include <hdf5.h>

#define DEFAULT_ORDER 16

struct adftool_file *
adftool_file_alloc (void)
{
  ensure_init ();
  struct adftool_file *ret = malloc (sizeof (struct adftool_file));
  if (ret != NULL)
    {
      ret->hdf5_file = H5I_INVALID_HID;
    }
  return ret;
}

void
adftool_file_free (struct adftool_file *file)
{
  adftool_file_close (file);
  free (file);
}

static int
dictionary_get_key (const struct adftool_file *file,
		    const struct adftool_bplus_key *key, size_t *length,
		    char **dest)
{
  int error = 0;
  uint32_t known;
  void *unknown;
  if (key_get_unknown (key, &unknown) == 0)
    {
      /* The key is unknown. */
      struct literal *lit = unknown;
      *length = lit->length;
      *dest = malloc (lit->length + 1);
      if (*dest == NULL)
	{
	  /* FIXME: Maybe don’t load the entire key? Find a clever way
	     to compare them bit by bit? */
	  abort ();
	}
      memcpy (*dest, lit->data, *length);
      (*dest)[*length] = '\0';
    }
  else if (key_get_known (key, &known) == 0)
    {
      size_t known_length = 0;
      /* Fast strings are at most 12 bytes. */
      *dest = malloc (13);
      if (*dest == NULL)
	{
	  /* FIXME: same. */
	  abort ();
	}
      error =
	adftool_dictionary_get (file, known, 0, 13, &known_length, *dest);
      if (error)
	{
	  free (*dest);
	  goto wrapup;
	}
      *length = known_length;
      if (*length > 12)
	{
	  char *bigger_dest = realloc (*dest, *length + 1);
	  if (bigger_dest == NULL)
	    {
	      /* FIXME: same. */
	      abort ();
	    }
	  *dest = bigger_dest;
	}
      error =
	adftool_dictionary_get (file, known, 0, *length + 1, &known_length,
				*dest);
      if (error || known_length != *length)
	{
	  free (*dest);
	  goto wrapup;
	}
    }
  else
    {
      abort ();
    }
wrapup:
  return error;
}

static int
dictionary_compare (const struct adftool_bplus_key *key_a,
		    const struct adftool_bplus_key *key_b,
		    int *result, void *context)
{
  struct adftool_file *file = context;
  size_t a_length, b_length;
  char *a = NULL;
  char *b = NULL;
  int error_a = dictionary_get_key (file, key_a, &a_length, &a);
  int error_b = dictionary_get_key (file, key_b, &b_length, &b);
  if (error_a || error_b)
    {
      if (error_a == 0)
	{
	  free (a);
	}
      if (error_b == 0)
	{
	  free (b);
	}
    }
  else
    {
      if (a_length < b_length)
	{
	  *result = -1;
	}
      else if (a_length > b_length)
	{
	  *result = 1;
	}
      else
	{
	  *result = memcmp (a, b, a_length);
	}
    }
  return (error_a || error_b);
}

int
adftool_file_open (struct adftool_file *file, const char *filename, int write)
{
  int error = 0;
  adftool_file_close (file);
  hid_t hdf5_file = H5I_INVALID_HID;
  hid_t dictionary_group = H5I_INVALID_HID;
  hid_t dictionary_bplus_dataset = H5I_INVALID_HID;
  hid_t dictionary_bplus_nextid = H5I_INVALID_HID;
  hid_t dictionary_strings_dataset = H5I_INVALID_HID;
  hid_t dictionary_strings_nextid = H5I_INVALID_HID;
  struct bplus dictionary_bplus;
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
	  error = 1;
	  goto wrapup;
	}
    }
  dictionary_group = H5Gopen2 (hdf5_file, "/dictionary", H5P_DEFAULT);
  if (dictionary_group == H5I_INVALID_HID)
    {
      /* Try creating… It will fail if the file is not writable. */
      dictionary_group =
	H5Gcreate (hdf5_file, "/dictionary", H5P_DEFAULT, H5P_DEFAULT,
		   H5P_DEFAULT);
      if (dictionary_group == H5I_INVALID_HID)
	{
	  error = 1;
	  goto wrapup;
	}
    }
  dictionary_bplus_dataset =
    H5Dopen2 (hdf5_file, "/dictionary/keys", H5P_DEFAULT);
  if (dictionary_bplus_dataset == H5I_INVALID_HID)
    {
      hsize_t minimum_dimensions[] = { 1, (2 * DEFAULT_ORDER + 1) };
      hsize_t maximum_dimensions[] =
	{ H5S_UNLIMITED, (2 * DEFAULT_ORDER + 1) };
      hsize_t chunk_dimensions[] = { 1, 2 * DEFAULT_ORDER + 1 };
      hid_t fspace =
	H5Screate_simple (2, minimum_dimensions, maximum_dimensions);
      if (fspace == H5I_INVALID_HID)
	{
	  error = 1;
	  goto wrapup;
	}
      hid_t dataset_creation_properties = H5Pcreate (H5P_DATASET_CREATE);
      if (dataset_creation_properties == H5I_INVALID_HID)
	{
	  H5Sclose (fspace);
	  error = 1;
	  goto wrapup;
	}
      if (H5Pset_chunk (dataset_creation_properties, 2, chunk_dimensions) < 0)
	{
	  H5Sclose (fspace);
	  error = 1;
	  goto wrapup;
	}
      dictionary_bplus_dataset =
	H5Dcreate2 (hdf5_file, "/dictionary/keys", H5T_NATIVE_B32, fspace,
		    H5P_DEFAULT, dataset_creation_properties, H5P_DEFAULT);
      H5Pclose (dataset_creation_properties);
      H5Sclose (fspace);
      if (dictionary_bplus_dataset == H5I_INVALID_HID)
	{
	  error = 1;
	  goto wrapup;
	}
    }
  dictionary_bplus_nextid =
    H5Aopen (dictionary_bplus_dataset, "nextID", H5P_DEFAULT);
  if (dictionary_bplus_nextid == H5I_INVALID_HID)
    {
      hid_t fspace = H5Screate (H5S_SCALAR);
      if (fspace == H5I_INVALID_HID)
	{
	  error = 1;
	  goto wrapup;
	}
      dictionary_bplus_nextid =
	H5Acreate2 (dictionary_bplus_dataset, "nextID", H5T_NATIVE_INT,
		    fspace, H5P_DEFAULT, H5P_DEFAULT);
      H5Sclose (fspace);
      if (dictionary_bplus_nextid == H5I_INVALID_HID)
	{
	  error = 1;
	  goto wrapup;
	}
      int zero = 0;
      if (H5Awrite (dictionary_bplus_nextid, H5T_NATIVE_INT, &zero) < 0)
	{
	  error = 1;
	  goto wrapup;
	}
    }
  dictionary_strings_dataset =
    H5Dopen2 (hdf5_file, "/dictionary/strings", H5P_DEFAULT);
  if (dictionary_strings_dataset == H5I_INVALID_HID)
    {
      hsize_t minimum_dimensions[] = { 1, 13 };
      hsize_t maximum_dimensions[] = { H5S_UNLIMITED, 13 };
      hsize_t chunk_dimensions[] = { 1, 13 };
      hid_t fspace =
	H5Screate_simple (2, minimum_dimensions, maximum_dimensions);
      if (fspace == H5I_INVALID_HID)
	{
	  error = 1;
	  goto wrapup;
	}
      hid_t dataset_creation_properties = H5Pcreate (H5P_DATASET_CREATE);
      if (dataset_creation_properties == H5I_INVALID_HID)
	{
	  H5Sclose (fspace);
	  error = 1;
	  goto wrapup;
	}
      if (H5Pset_chunk (dataset_creation_properties, 2, chunk_dimensions) < 0)
	{
	  H5Sclose (fspace);
	  error = 1;
	  goto wrapup;
	}
      dictionary_strings_dataset =
	H5Dcreate2 (hdf5_file, "/dictionary/strings", H5T_NATIVE_B8, fspace,
		    H5P_DEFAULT, dataset_creation_properties, H5P_DEFAULT);
      H5Pclose (dataset_creation_properties);
      H5Sclose (fspace);
      if (dictionary_strings_dataset == H5I_INVALID_HID)
	{
	  error = 1;
	  goto wrapup;
	}
    }
  dictionary_strings_nextid =
    H5Aopen (dictionary_strings_dataset, "nextID", H5P_DEFAULT);
  if (dictionary_strings_nextid == H5I_INVALID_HID)
    {
      hid_t fspace = H5Screate (H5S_SCALAR);
      if (fspace == H5I_INVALID_HID)
	{
	  error = 1;
	  goto wrapup;
	}
      dictionary_strings_nextid =
	H5Acreate2 (dictionary_strings_dataset, "nextID", H5T_NATIVE_INT,
		    fspace, H5P_DEFAULT, H5P_DEFAULT);
      H5Sclose (fspace);
      if (dictionary_strings_nextid == H5I_INVALID_HID)
	{
	  error = 1;
	  goto wrapup;
	}
      int zero = 0;
      if (H5Awrite (dictionary_strings_nextid, H5T_NATIVE_INT, &zero) < 0)
	{
	  error = 1;
	  goto wrapup;
	}
    }
  error =
    bplus_from_hdf5 (&dictionary_bplus, dictionary_bplus_dataset,
		     dictionary_bplus_nextid);
  if (error)
    {
      goto wrapup;
    }
  bplus_set_compare (&dictionary_bplus, dictionary_compare, (void *) file);
wrapup:
  if (error == 0)
    {
      file->hdf5_file = hdf5_file;
      file->dictionary.group = dictionary_group;
      file->dictionary.bplus_dataset = dictionary_bplus_dataset;
      file->dictionary.bplus_nextid = dictionary_bplus_nextid;
      memcpy (&(file->dictionary.bplus), &dictionary_bplus,
	      sizeof (struct bplus));
      file->dictionary.strings_dataset = dictionary_strings_dataset;
      file->dictionary.strings_nextid = dictionary_strings_nextid;
    }
  else
    {
      if (dictionary_strings_nextid != H5I_INVALID_HID)
	{
	  H5Aclose (dictionary_strings_nextid);
	}
      if (dictionary_strings_dataset != H5I_INVALID_HID)
	{
	  H5Dclose (dictionary_strings_dataset);
	}
      if (dictionary_bplus_nextid != H5I_INVALID_HID)
	{
	  H5Aclose (dictionary_bplus_nextid);
	}
      if (dictionary_bplus_dataset != H5I_INVALID_HID)
	{
	  H5Dclose (dictionary_bplus_dataset);
	}
      if (dictionary_group != H5I_INVALID_HID)
	{
	  H5Gclose (dictionary_group);
	}
      if (hdf5_file != H5I_INVALID_HID)
	{
	  H5Fclose (hdf5_file);
	}
    }
  return error;
}

void
adftool_file_close (struct adftool_file *file)
{
  if (file->hdf5_file != H5I_INVALID_HID)
    {
      H5Aclose (file->dictionary.strings_nextid);
      H5Dclose (file->dictionary.strings_dataset);
      H5Aclose (file->dictionary.bplus_nextid);
      H5Dclose (file->dictionary.bplus_dataset);
      H5Gclose (file->dictionary.group);
      H5Fclose (file->hdf5_file);
      file->hdf5_file = H5I_INVALID_HID;
    }
}