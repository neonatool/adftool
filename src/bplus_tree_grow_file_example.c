#include <adftool_private.h>
#include <adftool_bplus.h>
#include <stdio.h>

#define _(String) gettext (String)
#define N_(String) (String)

/* Now we take the same example as _grow_example.c, but we build it in
   an HDF5 file. */

static hid_t file;
static hid_t dataset;
static hid_t nextID;

static struct bplus *bplus = NULL;

static const char *filename = "bplus-tree-grow-file-example.hdf5";

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
  int nextID_value = 3;
  int write_error = H5Awrite (nextID, H5T_NATIVE_INT, &nextID_value);
  if (write_error < 0)
    {
      fprintf (stderr, _("Could not set the attribute to a scalar.\n"));
      abort ();
    }
  H5Sclose (fspace);
  /* Now populate it. */
  static const uint32_t initial_data[] = {
    /* */ 2, -1, -1,
    /* */ 1, 2, 0, 0,
    /* */ -1, 0,

    0, 1, 2,
    0, 1, 2, 2,
    0, 1 << 31,

    3, 4, 5,
    3, 4, 5, 0,
    0, 1 << 31
  };
  hsize_t file_size[2];
  file_size[0] = sizeof (initial_data) / (9 * sizeof (initial_data[0]));
  file_size[1] = 9;
  hid_t selection_space = H5Screate_simple (2, file_size, NULL);
  if (selection_space == H5I_INVALID_HID)
    {
      fprintf (stderr, _("Could not create \
the initialized file space.\n"));
      abort ();
    }
  hsize_t initial_data_size =
    sizeof (initial_data) / sizeof (initial_data[0]);
  hid_t initial_data_space = H5Screate_simple (1, &initial_data_size, NULL);
  if (initial_data_space == H5I_INVALID_HID)
    {
      fprintf (stderr, _("Could not create \
the initial data space.\n"));
      abort ();
    }
  int setup_error =
    H5Dwrite (dataset, H5T_NATIVE_B32, initial_data_space, selection_space,
	      H5P_DEFAULT,
	      initial_data);
  if (setup_error < 0)
    {
      fprintf (stderr, _("Could not initialize \
the file with the initial data.\n"));
      abort ();
    }
  H5Sclose (initial_data_space);
  H5Sclose (selection_space);
  static struct bplus my_bplus;
  bplus = &my_bplus;
  int set_bplus_error = bplus_from_hdf5 (bplus, dataset, nextID);
  if (set_bplus_error)
    {
      fprintf (stderr, _("Could not set up the bplus.\n"));
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

/* This is what we would like to get at the end: */
static const uint32_t expected[][9] = {
  /* This is the new hat on top of the old root, the old root (its
     only child) is now ID 3. */
  {
   /* */ -1, -1, -1,
   /* */ 3, 0, 0, 0,
   /* */ -1, 0
   },
  /* The leaves have updated their parent, it is now number 3. */
  {
   0, 1, 2,
   0, 1, 2, 2,
   3, 1 << 31},
  {
   3, 4, 5,
   3, 4, 5, 0,
   3, 1 << 31},
  /* The old root has now 0 as a parent. */
  {
   /* */ 2, -1, -1,
   /* */ 1, 2, 0, 0,
   /* */ 0, 0
   }
};

static void
check (void)
{
  file = H5Fopen (filename, H5F_ACC_RDONLY, H5P_DEFAULT);
  if (file == H5I_INVALID_HID)
    {
      fprintf (stderr, _("Could not reopen the file for reading.\n"));
      abort ();
    }
  dataset = H5Dopen2 (file, "B+", H5P_DEFAULT);
  if (dataset == H5I_INVALID_HID)
    {
      fprintf (stderr, _("Could not reopen the dataset.\n"));
      abort ();
    }
  nextID = H5Aopen (dataset, "nextID", H5P_DEFAULT);
  if (nextID == H5I_INVALID_HID)
    {
      fprintf (stderr, _("Could not reopen nextID.\n"));
      abort ();
    }
  hid_t dataset_space = H5Dget_space (dataset);
  if (dataset_space == H5I_INVALID_HID)
    {
      fprintf (stderr, _("Could not get a handle \
of the dataset space.\n"));
      abort ();
    }
  int rank = H5Sget_simple_extent_ndims (dataset_space);
  if (rank != 2)
    {
      fprintf (stderr, _("Could not count the matrix dimensions, \
or it is not a matrix.\n"));
      abort ();
    }
  hsize_t true_dims[2];
  int check_rank = H5Sget_simple_extent_dims (dataset_space, true_dims, NULL);
  if (check_rank != rank)
    {
      fprintf (stderr, _("Inconsistent number of dimensions.\n"));
      abort ();
    }
  if (true_dims[0] < sizeof (expected) / sizeof (expected[0]))
    {
      fprintf (stderr, _("The matrix has not grown at all.\n"));
    }
  int next_id_value;
  if (H5Aread (nextID, H5T_NATIVE_INT, &next_id_value) < 0)
    {
      fprintf (stderr, _("Failed to read the next ID.\n"));
      abort ();
    }
  assert (next_id_value == sizeof (expected) / sizeof (expected[0]));
  assert (true_dims[1] == sizeof (expected[0]) / sizeof (expected[0][0]));
  uint32_t *actual_data =
    malloc (true_dims[0] * true_dims[1] * sizeof (uint32_t));
  if (actual_data == NULL)
    {
      fprintf (stderr, _("Failed to allocate the buffer.\n"));
      abort ();
    }
  if (H5Dread (dataset, H5T_NATIVE_B32, H5S_ALL, dataset_space, H5P_DEFAULT,
	       actual_data) < 0)
    {
      fprintf (stderr, _("Failed to read the data.\n"));
      abort ();
    }
  for (size_t i = 0; i < sizeof (expected) / sizeof (expected[0]); i++)
    {
      for (size_t j = 0; j < sizeof (expected[0]) / sizeof (expected[0][0]);
	   j++)
	{
	  size_t k = i * true_dims[1] + j;
	  if (actual_data[k] != expected[i][j])
	    {
	      fprintf (stderr, _("There is a mismatch.\n"));
	      abort ();
	    }
	}
    }
  H5Sclose (dataset_space);
  free (actual_data);
}

int
main ()
{
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
  prepare_file ();
  int error = bplus_grow (bplus);
  if (error)
    {
      fprintf (stderr, _("Error growing the tree.\n"));
      abort ();
    }
  finalize_file ();
  check ();
  fprintf (stderr, _("Growing a tree in an HDF5 file is OK!\n"));
  return 0;
}
