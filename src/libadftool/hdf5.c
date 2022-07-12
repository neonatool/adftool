#include <adftool_private.h>

static int
true_fetch (uint32_t node_id, size_t *row_actual_length,
	    size_t request_start, size_t request_length,
	    uint32_t * response, void *ctx)
{
  struct context *context = ctx;
  hid_t dataset_space = H5Dget_space (context->dataset);
  /* Get the true dimensions */
  int rank = H5Sget_simple_extent_ndims (dataset_space);
  if (rank < 0)
    {
      H5Sclose (dataset_space);
      return 1;
    }
  assert (rank == 2);
  hsize_t true_dims[2];
  int check_rank = H5Sget_simple_extent_dims (dataset_space, true_dims, NULL);
  if (check_rank < 0)
    {
      H5Sclose (dataset_space);
      return 1;
    }
  assert (rank == check_rank);
  /* Restrict to the row we want */
  hsize_t start[2] = { 0, 0 };
  start[0] = node_id;
  hsize_t count[2] = { 1, 0 };
  count[1] = true_dims[1];
  herr_t rowspace_error =
    H5Sselect_hyperslab (dataset_space, H5S_SELECT_SET, start, NULL, count,
			 NULL);
  if (rowspace_error < 0)
    {
      H5Sclose (dataset_space);
      return 1;
    }
  uint32_t *full_row = malloc (true_dims[1] * sizeof (uint32_t));
  if (full_row == NULL)
    {
      abort ();
    }
  herr_t read_error =
    H5Dread (context->dataset, H5T_NATIVE_B32, dataset_space, H5S_ALL,
	     H5P_DEFAULT, full_row);
  H5Sclose (dataset_space);
  if (read_error < 0)
    {
      free (full_row);
      return 1;
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
  free (full_row);
  return 0;
}

void
adftool_bplus_parameters_from_hdf5 (struct adftool_bplus_parameters
				    *parameters, hid_t dataset, hid_t next_id)
{
  parameters->fetch = true_fetch;
  parameters->fetch_context.type = HDF5;
  parameters->fetch_context.arg.hdf5.dataset = dataset;
  parameters->fetch_context.arg.hdf5.nextID = next_id;
}
