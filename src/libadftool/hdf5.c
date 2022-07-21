#include <adftool_private.h>
#include <adftool_bplus_node.h>

/* FIXME: The data is read and written as H5T_NATIVE_B32, which is
   incorrect because there’s no endianness conversion. */

static int
true_fetch (uint32_t node_id, size_t *row_actual_length,
	    size_t request_start, size_t request_length,
	    uint32_t * response, void *ctx)
{
  int error = 0;
  struct hdf5_context *context = ctx;
  hid_t dataset_space = H5Dget_space (context->dataset);
  /* Get the true dimensions */
  int rank = H5Sget_simple_extent_ndims (dataset_space);
  if (rank < 0)
    {
      error = 1;
      goto cleanup_dataset_space;
    }
  assert (rank == 2);
  hsize_t true_dims[2];
  int check_rank = H5Sget_simple_extent_dims (dataset_space, true_dims, NULL);
  if (check_rank < 0)
    {
      error = 1;
      goto cleanup_dataset_space;
    }
  assert (rank == check_rank);
  /* Restrict to the row we want */
  hsize_t start[2] = { 0, 0 };
  start[0] = node_id;
  hsize_t count[2] = { 1, 0 };
  count[1] = true_dims[1];
  hid_t selection_space = H5Screate_simple (2, true_dims, NULL);
  if (selection_space == H5I_INVALID_HID)
    {
      error = 1;
      goto cleanup_dataset_space;
    }
  if (H5Sselect_hyperslab
      (selection_space, H5S_SELECT_AND, start, NULL, count, NULL) < 0)
    {
      error = 1;
      goto cleanup_selection_space;
    }
  hsize_t memory_length = true_dims[1];
  hid_t memory_space = H5Screate_simple (1, &memory_length, NULL);
  if (memory_space == H5I_INVALID_HID)
    {
      error = 1;
      goto cleanup_selection_space;
    }
  uint32_t *full_row = malloc (true_dims[1] * sizeof (uint32_t));
  if (full_row == NULL)
    {
      error = 1;
      goto cleanup_memory_space;
    }
  herr_t read_error =
    H5Dread (context->dataset, H5T_NATIVE_B32, memory_space, selection_space,
	     H5P_DEFAULT, full_row);
  if (read_error < 0)
    {
      error = 1;
      goto cleanup_full_row;
    }
  *row_actual_length = true_dims[1];
  size_t i;
  size_t request_end = request_start + request_length;
  for (i = 0; i < true_dims[1]; i++)
    {
      if (i >= request_start && i < request_end)
	{
	  response[i - request_start] = full_row[i];
	}
    }
cleanup_full_row:
  free (full_row);
cleanup_memory_space:
  H5Sclose (memory_space);
cleanup_selection_space:
  H5Sclose (selection_space);
cleanup_dataset_space:
  H5Sclose (dataset_space);
  return error;
}

static void
true_allocate (uint32_t * new_id, void *ctx)
{
  struct hdf5_context *context = ctx;
  unsigned int next_id_value;
  if (H5Aread (context->nextID, H5T_NATIVE_UINT, &next_id_value) < 0)
    {
      fprintf (stderr, _("Failed to read the next ID.\n"));
      abort ();
    }
  /* If nextID is equal to the number of rows, we must grow the
     dataset. */
  hid_t dataset_space = H5Dget_space (context->dataset);
  int rank = H5Sget_simple_extent_ndims (dataset_space);
  if (rank != 2)
    {
      fprintf (stderr, _("The dataset must be a matrix.\n"));
      abort ();
    }
  hsize_t true_dims[2];
  int check_rank = H5Sget_simple_extent_dims (dataset_space, true_dims, NULL);
  H5Sclose (dataset_space);
  if (check_rank != 2)
    {
      fprintf (stderr, _("Inconsistent result of the dataset dimensions.\n"));
      abort ();
    }
  if (next_id_value == true_dims[0])
    {
      /* We must grow the dataset. */
      true_dims[0] *= 2;
      if (true_dims[0] == 0)
	{
	  true_dims[0] = 1;
	}
      if (H5Dset_extent (context->dataset, true_dims) < 0)
	{
	  fprintf (stderr, _("Could not extend the dataset.\n"));
	  abort ();
	}
    }
  *new_id = next_id_value++;
  if (H5Awrite (context->nextID, H5T_NATIVE_UINT, &next_id_value) < 0)
    {
      fprintf (stderr, _("Failed to update the next ID.\n"));
      abort ();
    }
}

static void
true_store (uint32_t row_id, size_t rq_start, size_t rq_length,
	    const uint32_t * row, void *ctx)
{
  struct hdf5_context *context = ctx;
  /* Get the true dimensions */
  hid_t dataset_space = H5Dget_space (context->dataset);
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
  if (rq_start >= true_dims[1])
    {
      rq_start = 0;
      rq_length = 0;
    }
  else if (rq_start + rq_length >= true_dims[1])
    {
      rq_length = true_dims[1] - rq_start;
    }
  assert (rq_start <= true_dims[1]);
  if (true_dims[1] != 0)
    {
      assert (rq_start < true_dims[1]);
    }
  assert (rq_start + rq_length <= true_dims[1]);
  /* Restrict to the part of the row we want to write */
  hsize_t start[2] = { 0, 0 };
  start[0] = row_id;
  start[1] = rq_start;
  hsize_t count[2] = { 1, 0 };
  count[1] = rq_length;
  hid_t selection_space = H5Screate_simple (2, true_dims, NULL);
  if (selection_space == H5I_INVALID_HID)
    {
      fprintf (stderr, _("Could not define the selection space.\n"));
      abort ();
    }
  if (H5Sselect_hyperslab
      (selection_space, H5S_SELECT_AND, start, NULL, count, NULL) < 0)
    {
      fprintf (stderr, _("Could not select that row.\n"));
      abort ();
    }
  hsize_t memory_length = rq_length;
  hid_t memory_space = H5Screate_simple (1, &memory_length, NULL);
  if (memory_space == H5I_INVALID_HID)
    {
      fprintf (stderr, _("Could not define the memory space.\n"));
      abort ();
    }
  herr_t write_error =
    H5Dwrite (context->dataset, H5T_NATIVE_B32, memory_space, selection_space,
	      H5P_DEFAULT, row);
  if (write_error < 0)
    {
      fprintf (stderr, _("Could not update the file.\n"));
      abort ();
    }
  H5Sclose (selection_space);
  H5Sclose (memory_space);
  H5Sclose (dataset_space);
}

int
adftool_bplus_from_hdf5 (struct adftool_bplus *bplus, hid_t dataset,
			 hid_t next_id)
{
  bplus->fetch = true_fetch;
  bplus->fetch_context.type = HDF5;
  bplus->fetch_context.arg.hdf5.dataset = dataset;
  bplus->fetch_context.arg.hdf5.nextID = next_id;
  bplus->allocate = true_allocate;
  bplus->allocate_context.type = HDF5;
  bplus->allocate_context.arg.hdf5.dataset = dataset;
  bplus->allocate_context.arg.hdf5.nextID = next_id;
  bplus->store = true_store;
  bplus->store_context.type = HDF5;
  bplus->store_context.arg.hdf5.dataset = dataset;
  bplus->store_context.arg.hdf5.nextID = next_id;
  int next_id_value;
  if (H5Aread (next_id, H5T_NATIVE_INT, &next_id_value) < 0)
    {
      return 1;
    }
  if (next_id_value == 0)
    {
      /* Allocate an empty root. */
      uint32_t new_root;
      struct node root_node;
      adftool_bplus_allocate (bplus, &new_root);
      size_t row_length;
      int query_rank_error =
	adftool_bplus_fetch (bplus, new_root, &row_length, 0, 0, NULL);
      if (query_rank_error)
	{
	  /* FIXME: revert next_id to 0? */
	  return -1;
	}
      assert ((row_length % 2) == 1);
      size_t order = ((row_length - 1) / 2);
      if (node_init (order, new_root, &root_node) != 0)
	{
	  return 1;
	}
      node_set_leaf (&root_node);
      for (size_t i = 0; i + 1 < order; i++)
	{
	  node_set_key (&root_node, i, ((uint32_t) (-1)));
	  node_set_value (&root_node, i, 0);
	}
      node_set_next_leaf (&root_node, 0);
      node_set_parent (&root_node, ((uint32_t) (-1)));
      node_store (bplus, &root_node);
      node_clean (&root_node);
    }
  return 0;
}
