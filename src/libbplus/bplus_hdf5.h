#ifndef H_BPLUS_HDF5_INCLUDED
# define H_BPLUS_HDF5_INCLUDED

# include <bplus.h>

# include <stdlib.h>
# include <string.h>

struct bplus_hdf5_table;

static inline struct bplus_hdf5_table *hdf5_table_alloc (void);

static inline void hdf5_table_free (struct bplus_hdf5_table *table);

static inline int hdf5_table_set (struct bplus_hdf5_table *table, hid_t data);

static inline size_t hdf5_table_order (struct bplus_hdf5_table *table);

static inline hid_t hdf5_table_type (const struct bplus_hdf5_table *table);

  /* I provide here a set of convenience callbacks for working with
     HDF5 tables. The first argument is of type struct
     hdf5_table, but gcc emits a warning if we do that. */

static inline
  int hdf5_fetch (void *table, size_t row,
		  size_t start, size_t length, size_t *actual_length,
		  uint32_t * data);

static inline void hdf5_allocate (void *table, uint32_t * new_id);

static inline
  void hdf5_update (void *table, size_t row,
		    size_t start, size_t length, const uint32_t * data);

# include "bplus_prime.h"
# include <hdf5.h>

struct bplus_hdf5_table
{
  hid_t dataset;
  hid_t nextID;
  /* HDF5 does not provide a type that maps to uint32_t. */
  hid_t native_u32_type;
  size_t order;
};

static inline struct bplus_hdf5_table *
hdf5_table_alloc (void)
{
  hid_t native_u32_type = H5Tcopy (H5T_NATIVE_UINT);
  if (native_u32_type == H5I_INVALID_HID)
    {
      return NULL;
    }
  /* native_u32_type is not yet 4 bytes, it’s sizeof (unsigned int)
     bytes… */
  if (H5Tset_size (native_u32_type, 4) < 0)
    {
      H5Tclose (native_u32_type);
      return NULL;
    }
  struct bplus_hdf5_table *ret = malloc (sizeof (struct bplus_hdf5_table));
  if (ret != NULL)
    {
      ret->dataset = H5I_INVALID_HID;
      ret->nextID = H5I_INVALID_HID;
      ret->native_u32_type = native_u32_type;
      ret->order = 0;
    }
  else
    {
      H5Tclose (native_u32_type);
    }
  return ret;
}

static inline void
hdf5_table_free (struct bplus_hdf5_table *table)
{
  if (table != NULL)
    {
      H5Dclose (table->dataset);
      H5Aclose (table->nextID);
      H5Tclose (table->native_u32_type);
      table->dataset = H5I_INVALID_HID;
      table->nextID = H5I_INVALID_HID;
    }
  free (table);
}

static int
hdf5_table_get_dimensions (hid_t dataset, size_t *nrows, size_t *ncols,
			   hsize_t * maxrows, size_t *order)
{
  int ret = 0;
  hid_t dataset_space = H5Dget_space (dataset);
  if (dataset_space == H5I_INVALID_HID)
    {
      ret = 1;
      goto cleanup;
    }
  size_t rank = H5Sget_simple_extent_ndims (dataset_space);
  if (rank != 2)
    {
      ret = 1;
      goto cleanup_ds;
    }
  hsize_t dims[2];
  hsize_t maxdims[2];
  if (H5Sget_simple_extent_dims (dataset_space, dims, maxdims) < 0)
    {
      ret = 1;
      goto cleanup_ds;
    }
  if (maxdims[1] != dims[1])
    {
      ret = 1;
      goto cleanup_ds;
    }
  if (dims[1] % 2 != 1)
    {
      ret = 1;
      goto cleanup_ds;
    }
  *nrows = dims[0];
  *ncols = dims[1];
  *maxrows = maxdims[0];
  *order = (dims[1] - 1) / 2;
cleanup_ds:
  H5Sclose (dataset_space);
cleanup:
  return ret;
}

static inline int
hdf5_table_set (struct bplus_hdf5_table *table, hid_t data)
{
  int ret = 0;
  H5Dclose (table->dataset);
  H5Aclose (table->nextID);
  table->dataset = data;
  table->nextID = H5Aopen (data, "nextID", H5P_DEFAULT);
  if (table->nextID == H5I_INVALID_HID)
    {
      hid_t acpl = H5Pcreate (H5P_ATTRIBUTE_CREATE);
      if (acpl == H5I_INVALID_HID)
	{
	  ret = 1;
	  goto cleanup;
	}
      hid_t fspace = H5Screate (H5S_SCALAR);
      if (fspace == H5I_INVALID_HID)
	{
	  ret = 1;
	  goto cleanup_acpl;
	}
      table->nextID =
	H5Acreate2 (data, "nextID", table->native_u32_type, fspace, acpl,
		    H5P_DEFAULT);
      if (table->nextID == H5I_INVALID_HID)
	{
	  ret = 1;
	  goto cleanup_fspace;
	}
      uint32_t value = 0;
      if (H5Awrite (table->nextID, table->native_u32_type, &value) < 0)
	{
	  ret = 1;
	  goto cleanup_fspace;
	}
    cleanup_fspace:
      H5Sclose (fspace);
    cleanup_acpl:
      H5Pclose (acpl);
    }
  uint32_t next_id_initial_value;
  if (H5Aread (table->nextID, table->native_u32_type, &next_id_initial_value)
      < 0)
    {
      ret = 1;
      goto cleanup;
    }
  if (next_id_initial_value == 0)
    {
      /* Need to prime the table. */
      /* First, get the order by looking at the table dimensions. */
      size_t nrows, ncols, order;
      hsize_t maxrows;
      int dims_error =
	hdf5_table_get_dimensions (data, &nrows, &ncols, &maxrows, &order);
      if (dims_error != 0)
	{
	  ret = 1;
	  goto cleanup;
	}
      if (maxrows != H5S_UNLIMITED && maxrows < 1)
	{
	  ret = 1;
	  goto cleanup;
	}
      if (nrows == 0)
	{
	  /* First, make sure that there is 1 row. */
	  hsize_t new_dims[2] = { 1, 2 * order + 1 };
	  if (H5Dset_extent (table->dataset, new_dims) < 0)
	    {
	      ret = 1;
	      goto cleanup;
	    }
	}
      /* Set the first row */
      uint32_t *row0 = malloc ((2 * order + 1) * sizeof (uint32_t));
      if (row0 == NULL)
	{
	  ret = 1;
	  goto cleanup;
	}
      prime (order, row0);
      hid_t prime_fspace = H5Dget_space (table->dataset);
      if (prime_fspace == H5I_INVALID_HID)
	{
	  ret = 1;
	  goto cleanup_row0;
	}
      hsize_t top_left_corner[2] = { 0, 0 };
      hsize_t dims_one_row[2] = { 1, 2 * order + 1 };
      if (H5Sselect_hyperslab
	  (prime_fspace, H5S_SELECT_SET, top_left_corner, NULL, dims_one_row,
	   NULL) < 0)
	{
	  ret = 1;
	  goto cleanup_prime_fspace;
	}
      if (H5Dwrite
	  (table->dataset, table->native_u32_type, H5S_ALL, prime_fspace,
	   H5P_DEFAULT, row0) < 0)
	{
	  ret = 1;
	  goto cleanup_prime_fspace;
	}
      /* Update nextID */
      unsigned int next_id_value = 1;
      if (H5Awrite (table->nextID, table->native_u32_type, &next_id_value) <
	  0)
	{
	  ret = 1;
	  goto cleanup_prime_fspace;
	}
    cleanup_prime_fspace:
      H5Sclose (prime_fspace);
    cleanup_row0:
      free (row0);
      if (ret != 0)
	{
	  goto cleanup;
	}
    }
  /* We still need to get the table order. */
  size_t nrows, ncols, order;
  hsize_t maxrows;
  int order_error =
    hdf5_table_get_dimensions (table->dataset, &nrows, &ncols, &maxrows,
			       &order);
  if (order_error)
    {
      ret = 1;
      goto cleanup;
    }
  table->order = order;
cleanup:
  if (ret != 0)
    {
      H5Aclose (table->nextID);
      table->dataset = H5I_INVALID_HID;
      table->nextID = H5I_INVALID_HID;
    }
  return ret;
}

static inline size_t
hdf5_table_order (struct bplus_hdf5_table *table)
{
  return table->order;
}

static inline int
hdf5_fetch (void *_table, size_t row, size_t start,
	    size_t length, size_t *actual_length, uint32_t * data)
{
  struct bplus_hdf5_table *table = _table;
  int error = 0;
  /* Get the true dimensions */
  size_t nrows, ncols, order;
  hsize_t maxrows;
  int dims_error =
    hdf5_table_get_dimensions (table->dataset, &nrows, &ncols, &maxrows,
			       &order);
  if (dims_error != 0)
    {
      error = 1;
      goto cleanup;
    }
  *actual_length = ncols;
  /* Restrict to the row we want */
  hsize_t true_dims[2] = { nrows, ncols };
  hsize_t init[2] = { 0, 0 };
  init[0] = row;
  init[1] = start;
  hsize_t count[2] = { 1, 0 };
  if (start < true_dims[1])
    {
      count[1] = true_dims[1] - start;
    }
  if (count[1] > length)
    {
      count[1] = length;
    }
  hid_t selection_space = H5Screate_simple (2, true_dims, NULL);
  if (selection_space == H5I_INVALID_HID)
    {
      error = 1;
      goto cleanup;
    }
  if (H5Sselect_hyperslab
      (selection_space, H5S_SELECT_AND, init, NULL, count, NULL) < 0)
    {
      error = 1;
      goto cleanup_selection_space;
    }
  hsize_t memory_length = length;
  if (memory_length > count[1])
    {
      memory_length = count[1];
    }
  hid_t memory_space = H5Screate_simple (1, &memory_length, NULL);
  if (memory_space == H5I_INVALID_HID)
    {
      error = 1;
      goto cleanup_selection_space;
    }
  herr_t read_error =
    H5Dread (table->dataset, table->native_u32_type, memory_space,
	     selection_space,
	     H5P_DEFAULT, data);
  if (read_error < 0)
    {
      error = 1;
      goto cleanup_memory_space;
    }
cleanup_memory_space:
  H5Sclose (memory_space);
cleanup_selection_space:
  H5Sclose (selection_space);
cleanup:
  return error;
}

static inline void
hdf5_allocate (void *_table, uint32_t * new_id)
{
  struct bplus_hdf5_table *table = _table;
  uint32_t next_id_value;
  if (H5Aread (table->nextID, table->native_u32_type, &next_id_value) < 0)
    {
      *new_id = ((uint32_t) (-1));
      return;
    }
  /* If nextID is equal to the number of rows, we must grow the
     dataset. */
  size_t nrows, ncols, order;
  hsize_t maxrows;
  int dims_error =
    hdf5_table_get_dimensions (table->dataset, &nrows, &ncols, &maxrows,
			       &order);
  if (dims_error != 0)
    {
      return;
    }
  hsize_t true_dims[2] = { nrows, ncols };
  if (next_id_value == true_dims[0])
    {
      /* We must grow the dataset. */
      true_dims[0] *= 2;
      if (true_dims[0] == 0)
	{
	  true_dims[0] = 1;
	}
      if (H5Dset_extent (table->dataset, true_dims) < 0)
	{
	  *new_id = ((uint32_t) (-1));
	  return;
	}
    }
  *new_id = next_id_value++;
  H5Awrite (table->nextID, table->native_u32_type, &next_id_value);
}

static inline void
hdf5_update (void *_table, size_t row, size_t start,
	     size_t length, const uint32_t * data)
{
  struct bplus_hdf5_table *table = _table;
  /* Get the true dimensions */
  size_t nrows, ncols, order;
  hsize_t maxrows;
  int dims_error =
    hdf5_table_get_dimensions (table->dataset, &nrows, &ncols, &maxrows,
			       &order);
  if (dims_error != 0)
    {
      return;
    }
  /* Restrict to the row we want */
  hsize_t true_dims[2] = { nrows, ncols };
  hsize_t init[2] = { 0, 0 };
  init[0] = row;
  init[1] = start;
  hsize_t count[2] = { 1, 0 };
  if (start < true_dims[1])
    {
      count[1] = true_dims[1] - start;
    }
  if (count[1] > length)
    {
      count[1] = length;
    }
  hid_t selection_space = H5Screate_simple (2, true_dims, NULL);
  if (selection_space == H5I_INVALID_HID)
    {
      return;
    }
  if (H5Sselect_hyperslab
      (selection_space, H5S_SELECT_AND, init, NULL, count, NULL) < 0)
    {
      goto cleanup_selection_space;
    }
  hsize_t memory_length = length;
  if (memory_length > count[1])
    {
      memory_length = count[1];
    }
  hid_t memory_space = H5Screate_simple (1, &memory_length, NULL);
  if (memory_space == H5I_INVALID_HID)
    {
      goto cleanup_selection_space;
    }
  herr_t write_error =
    H5Dwrite (table->dataset, table->native_u32_type, memory_space,
	      selection_space, H5P_DEFAULT, data);
  if (write_error < 0)
    {
      goto cleanup_memory_space;
    }
cleanup_memory_space:
  H5Sclose (memory_space);
cleanup_selection_space:
  H5Sclose (selection_space);
}

static inline hid_t
hdf5_table_type (const struct bplus_hdf5_table *table)
{
  return table->native_u32_type;
}

#endif /* not H_BPLUS_HDF5_INCLUDED */
