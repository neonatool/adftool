#include <config.h>

#include <attribute.h>
#include <adftool.h>
#include <hdf5.h>

#include <stdio.h>
#include <stdlib.h>
#include "gettext.h"
#include "relocatable.h"
#include "progname.h"
#include <locale.h>

#define _(String) gettext(String)
#define N_(String) (String)

static void prepare_files (void);

int
main (int argc, char *argv[])
{
  (void) argc;
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);
  /* We demonstrate the following things: */
  /* 1. How to open a well-formed file, for reading; */
  /* 2. How to open a well-formed file, for reading and writing; */
  /* 3. What happens if we open a non-existing file for reading; */
  /* 4. What happens if we open a non-existing file for writing; */
  /* 5. What happens if we open a corrupted file (valid HDF5, but not ADF) for reading; */
  /* 6. What happens if we open a corrupted file for writing. */
  prepare_files ();

  struct adftool_file *file;

  /* First, the well-formed file. */
  /* For reading: */
  file = adftool_file_open ("well-formed.adf", 0);
  if (file == NULL)
    {
      abort ();
    }
  /* When we’re done with the file, we can close it. */
  adftool_file_close (file);
  /* And now for reading and writing: */
  file = adftool_file_open ("well-formed.adf", 1);
  if (file == NULL)
    {
      abort ();
    }
  adftool_file_close (file);

  /* We can’t open the non-existing file for reading. */
  file = adftool_file_open ("non-existing.adf", 0);
  if (file)
    {
      abort ();
    }
  /* We don’t need to close the file if we couldn’t open it. We can if
     we want. */
#ifdef I_WANT_TO_CLOSE_FILES
  adftool_file_close (file);
#endif
  /* We can however open the non-existing file for writing, and it
     will be all set up so that we can open it again for reading. */
  file = adftool_file_open ("non-existing.adf", 1);
  if (file == NULL)
    {
      abort ();
    }
  adftool_file_close (file);
  file = adftool_file_open ("non-existing.adf", 0);
  if (file == NULL)
    {
      abort ();
    }
  adftool_file_close (file);

  /* Truth to be told, the corrupted file case works just like the
     non-existing file case. Since the HDF5 file is not corrupted
     (only the ADF layer is), we can just consider that there is
     simply no RDF in there. */
  file = adftool_file_open ("corrupted.adf", 0);
  if (file)
    {
      abort ();
    }
  file = adftool_file_open ("corrupted.adf", 1);
  if (file == NULL)
    {
      abort ();
    }
  adftool_file_close (file);
  file = adftool_file_open ("corrupted.adf", 0);
  if (file == NULL)
    {
      abort ();
    }
  adftool_file_close (file);
  return 0;
}

static void
prepare_files (void)
{
  hid_t native_u32_type = H5Tcopy (H5T_NATIVE_UINT);
  if (native_u32_type == H5I_INVALID_HID)
    {
      abort ();
    }
  /* native_u32_type is not yet 4 bytes, it’s sizeof (unsigned int)
     bytes… */
  if (H5Tset_size (native_u32_type, 4) < 0)
    {
      abort ();
    }
  hid_t well_formed_file, corrupted_file;
  well_formed_file =
    H5Fcreate ("well-formed.adf", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  corrupted_file =
    H5Fcreate ("corrupted.adf", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  if (well_formed_file == H5I_INVALID_HID
      || corrupted_file == H5I_INVALID_HID)
    {
      abort ();
    }
  /* The difference between well-formed and corrupted: */
  hid_t dictionary_group =
    H5Gcreate (well_formed_file, "/dictionary", H5P_DEFAULT, H5P_DEFAULT,
	       H5P_DEFAULT);
  if (dictionary_group == H5I_INVALID_HID)
    {
      abort ();
    }
  hsize_t minimum_dimensions[] = { 1, 33 };
  hsize_t maximum_dimensions[] = { H5S_UNLIMITED, 33 };
  hsize_t chunk_dimensions[] = { 1, 33 };
  hid_t fspace = H5Screate_simple (2, minimum_dimensions, maximum_dimensions);
  if (fspace == H5I_INVALID_HID)
    {
      abort ();
    }
  hid_t dataset_creation_properties = H5Pcreate (H5P_DATASET_CREATE);
  if (dataset_creation_properties == H5I_INVALID_HID)
    {
      abort ();
    }
  if (H5Pset_chunk (dataset_creation_properties, 2, chunk_dimensions) < 0)
    {
      abort ();
    }
  hid_t dictionary_bplus_dataset =
    H5Dcreate2 (well_formed_file, "/dictionary/keys", native_u32_type, fspace,
		H5P_DEFAULT, dataset_creation_properties, H5P_DEFAULT);
  H5Pclose (dataset_creation_properties);
  H5Sclose (fspace);
  if (dictionary_bplus_dataset == H5I_INVALID_HID)
    {
      abort ();
    }
  fspace = H5Screate (H5S_SCALAR);
  if (fspace == H5I_INVALID_HID)
    {
      abort ();
    }
  hid_t dictionary_bplus_nextid =
    H5Acreate2 (dictionary_bplus_dataset, "nextID", H5T_NATIVE_INT, fspace,
		H5P_DEFAULT, H5P_DEFAULT);
  H5Sclose (fspace);
  if (dictionary_bplus_nextid == H5I_INVALID_HID)
    {
      abort ();
    }
  int nextid_value = 0;
  if (H5Awrite (dictionary_bplus_nextid, H5T_NATIVE_INT, &nextid_value) < 0)
    {
      abort ();
    }
  minimum_dimensions[0] = 1;
  minimum_dimensions[1] = 13;
  maximum_dimensions[0] = H5S_UNLIMITED;
  maximum_dimensions[1] = 13;
  chunk_dimensions[0] = 1;
  chunk_dimensions[1] = 13;
  fspace = H5Screate_simple (2, minimum_dimensions, maximum_dimensions);
  if (fspace == H5I_INVALID_HID)
    {
      abort ();
    }
  dataset_creation_properties = H5Pcreate (H5P_DATASET_CREATE);
  if (dataset_creation_properties == H5I_INVALID_HID)
    {
      abort ();
    }
  if (H5Pset_chunk (dataset_creation_properties, 2, chunk_dimensions) < 0)
    {
      abort ();
    }
  hid_t dictionary_strings_dataset =
    H5Dcreate2 (well_formed_file, "/dictionary/strings", H5T_NATIVE_B8,
		fspace, H5P_DEFAULT, dataset_creation_properties,
		H5P_DEFAULT);
  H5Pclose (dataset_creation_properties);
  H5Sclose (fspace);
  if (dictionary_strings_dataset == H5I_INVALID_HID)
    {
      abort ();
    }
  fspace = H5Screate (H5S_SCALAR);
  if (fspace == H5I_INVALID_HID)
    {
      abort ();
    }
  hid_t dictionary_strings_nextid =
    H5Acreate2 (dictionary_strings_dataset, "nextID", H5T_NATIVE_INT, fspace,
		H5P_DEFAULT, H5P_DEFAULT);
  H5Sclose (fspace);
  if (dictionary_strings_nextid == H5I_INVALID_HID)
    {
      abort ();
    }
  nextid_value = 0;
  if (H5Awrite (dictionary_strings_nextid, H5T_NATIVE_INT, &nextid_value) < 0)
    {
      abort ();
    }
  H5Aclose (dictionary_strings_nextid);
  H5Dclose (dictionary_strings_dataset);
  H5Aclose (dictionary_bplus_nextid);
  H5Dclose (dictionary_bplus_dataset);
  H5Gclose (dictionary_group);
  H5Fclose (corrupted_file);
  H5Fclose (well_formed_file);
  remove ("non-existing.adf");

  /* The well-formed file is not fully prepared yet: there are 0 rows
     in the dictionary index, which is invalid. */

  struct adftool_file *file = adftool_file_open ("well-formed.adf", 1);
  if (file == NULL)
    {
      abort ();
    }
  adftool_file_close (file);
}
