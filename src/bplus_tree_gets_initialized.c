#include <adftool_private.h>
#include <adftool_bplus.h>
#include <stdio.h>

#define _(String) gettext (String)
#define N_(String) (String)

/* Here we show that if you use a file with an empty dataset, the root
   will automatically be created. */

static hid_t file;
static hid_t dataset;
static hid_t nextID;

static struct bplus *bplus = NULL;

static const char *filename = "bplus-tree-gets-initialized.hdf5";

static void
prepare_file (void)
{
  file = H5Fcreate (filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  if (file == H5I_INVALID_HID)
    {
      fprintf (stderr, _("Could not create the example file.\n"));
      abort ();
    }
  /* I want a rank of 4 (=> 9 columns), and I want to allocate as many
     nodes as necessary up to 16 rows (should be enough). You can
     create an extensible dataset here if you wish. */
  hsize_t initial_dimensions[] = { 16, 9 };
  hid_t fspace = H5Screate_simple (2, initial_dimensions, NULL);
  if (fspace == H5I_INVALID_HID)
    {
      fprintf (stderr, _("Could not create the dataset space.\n"));
      abort ();
    }
  dataset =
    H5Dcreate2 (file, "B+", H5T_NATIVE_B32, fspace, H5P_DEFAULT,
		H5P_DEFAULT, H5P_DEFAULT);
  if (dataset == H5I_INVALID_HID)
    {
      fprintf (stderr, _("Could not create the dataset within the file.\n"));
      abort ();
    }
  H5Sclose (fspace);
  fspace = H5Screate (H5S_SCALAR);
  if (fspace == H5I_INVALID_HID)
    {
      fprintf (stderr, _("Could not create the attribute space.\n"));
      abort ();
    }
  nextID =
    H5Acreate2 (dataset, "nextID", H5T_NATIVE_INT, fspace, H5P_DEFAULT,
		H5P_DEFAULT);
  if (nextID == H5I_INVALID_HID)
    {
      fprintf (stderr, _("Could not create the attribute.\n"));
      abort ();
    }
  int nextID_value = 0;
  int write_error = H5Awrite (nextID, H5T_NATIVE_INT, &nextID_value);
  if (write_error < 0)
    {
      fprintf (stderr, _("Could not set the attribute to 0.\n"));
      abort ();
    }
  H5Sclose (fspace);
  static struct bplus my_bplus;
  bplus = &my_bplus;
  int error = bplus_from_hdf5 (bplus, dataset, nextID);
  if (error)
    {
      fprintf (stderr, _("Could not use the file as the B+ backend.\n"));
      abort ();
    }
}

static void
finalize_file ()
{
  H5Aclose (nextID);
  H5Dclose (dataset);
  H5Fclose (file);
}

int
main ()
{
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
  prepare_file ();
  /* Now the file must have at least one row. */
  int nextID_value;
  if (H5Aread (nextID, H5T_NATIVE_INT, &nextID_value) < 0)
    {
      fprintf (stderr, _("Error reading nextID.\n"));
      abort ();
    }
  if (nextID_value != 1)
    {
      fprintf (stderr, _("The root has not been allocated.\n"));
      abort ();
    }
  uint32_t first_row[9];
  static const uint32_t expected_first_row[9] = {
    -1, -1, -1,
    0, 0, 0,
    0,
    -1,
    (1 << 31)
  };
  hid_t dataset_space = H5Dget_space (dataset);
  if (dataset_space == H5I_INVALID_HID)
    {
      fprintf (stderr, _("Cannot get the dataset dataspace.\n"));
      abort ();
    }
  hsize_t start[2] = { 0, 0 };
  hsize_t count[2] = { 1, 9 };
  if (H5Sselect_hyperslab
      (dataset_space, H5S_SELECT_AND, start, NULL, count, NULL) < 0)
    {
      fprintf (stderr, _("Cannot select the first row.\n"));
      abort ();
    }
  hsize_t request_length = 9;
  hid_t memory_space = H5Screate_simple (1, &request_length, NULL);
  if (memory_space == H5I_INVALID_HID)
    {
      fprintf (stderr, _("Cannot create a simple dataspace.\n"));
      abort ();
    }
  herr_t read_error =
    H5Dread (dataset, H5T_NATIVE_B32, memory_space, dataset_space,
	     H5P_DEFAULT, first_row);
  if (read_error < 0)
    {
      fprintf (stderr, _("Cannot read the first row.\n"));
      abort ();
    }
  H5Sclose (memory_space);
  H5Sclose (dataset_space);
  finalize_file ();
  for (size_t i = 0; i < 9; i++)
    {
      if (first_row[i] != expected_first_row[i])
	{
	  fprintf (stderr, _("The matrix \
has not been initialized properly.\n"));
	  abort ();
	}
    }
  return 0;
}
