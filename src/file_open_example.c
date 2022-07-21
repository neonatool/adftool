#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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

  /* The first step is to allocate a file object. It does not directly
     open a new file. */

  struct adftool_file *file = adftool_file_alloc ();

  /* There could be some dark magic spell cast on our program that
     makes malloc returns NULL (ugh). */
  if (file == NULL)
    {
      abort ();
    }

  /* First, the well-formed file. */
  /* For reading: */
  int error = adftool_file_open (file, "well-formed.adf", 0);
  if (error)
    {
      abort ();
    }
  /* When we’re done with the file, we can close it. */
  adftool_file_close (file);
  /* And now for reading and writing: */
  error = adftool_file_open (file, "well-formed.adf", 1);
  if (error)
    {
      abort ();
    }
  adftool_file_close (file);

  /* We can’t open the non-existing file for reading. */
  error = adftool_file_open (file, "non-existing.adf", 0);
  if (error == 0)
    {
      abort ();
    }
  /* We don’t need to close the file if we couldn’t open it. */
  /* We can however open the non-existing file for writing, and it
     will be all set up so that we can open it again for reading. */
  error = adftool_file_open (file, "non-existing.adf", 1);
  if (error)
    {
      abort ();
    }
  adftool_file_close (file);
  error = adftool_file_open (file, "non-existing.adf", 0);
  if (error)
    {
      abort ();
    }
  adftool_file_close (file);

  /* Truth to be told, the corrupted file case works just like the
     non-existing file case. Since the HDF5 file is not corrupted
     (only the ADF layer is), we can just consider that there is
     simply no RDF in there. */
  error = adftool_file_open (file, "corrupted.adf", 0);
  if (error == 0)
    {
      abort ();
    }
  error = adftool_file_open (file, "corrupted.adf", 1);
  if (error)
    {
      abort ();
    }
  adftool_file_close (file);
  error = adftool_file_open (file, "corrupted.adf", 0);
  if (error)
    {
      abort ();
    }
  adftool_file_close (file);

  /* At the end of the program, free the resources. */
  adftool_file_free (file);
  return 0;
}

static void
prepare_files (void)
{
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
  H5Gclose (dictionary_group);
  H5Fclose (corrupted_file);
  H5Fclose (well_formed_file);
  remove ("non-existing.adf");
}
