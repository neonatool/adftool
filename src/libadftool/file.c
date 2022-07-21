#include <adftool_private.h>

#include <hdf5.h>

struct adftool_file
{
  hid_t hdf5_file;
  hid_t dictionary_group;
};

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

int
adftool_file_open (struct adftool_file *file, const char *filename, int write)
{
  int error = 0;
  adftool_file_close (file);
  hid_t hdf5_file = H5I_INVALID_HID;
  hid_t dictionary_group = H5I_INVALID_HID;
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
wrapup:
  if (error == 0)
    {
      file->hdf5_file = hdf5_file;
      file->dictionary_group = dictionary_group;
    }
  else
    {
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
      H5Gclose (file->dictionary_group);
      H5Fclose (file->hdf5_file);
      file->hdf5_file = H5I_INVALID_HID;
    }
}
