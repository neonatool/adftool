#ifndef H_ADFTOOL_QUADS_INCLUDED
# define H_ADFTOOL_QUADS_INCLUDED

# include <adftool.h>
# include <bplus.h>
# include <bplus_hdf5.h>
# include <hdf5.h>

# include "dictionary_index.h"
# include "statement.h"

# include <stdlib.h>
# include <assert.h>
# include <string.h>
# include <locale.h>
# include <stdbool.h>

# include "gettext.h"

# ifdef BUILDING_LIBADFTOOL
#  define _(String) dgettext (PACKAGE, (String))
#  define N_(String) (String)
# else
#  define _(String) gettext (String)
#  define N_(String) (String)
# endif

struct adftool_quads;

static struct adftool_quads *adftool_quads_alloc (void);
static void adftool_quads_free (struct adftool_quads *quads);

static int adftool_quads_setup (struct adftool_quads *quads, hid_t file);

static int quads_get (struct adftool_quads *quads,
		      struct adftool_dictionary_index *dictionary,
		      uint32_t id, struct adftool_statement *statement);

static int quads_delete (struct adftool_quads *quads,
			 struct adftool_dictionary_index *dictionary,
			 uint32_t id, uint64_t deletion_date);

static int quads_insert (struct adftool_quads *quads,
			 struct adftool_dictionary_index *dictionary,
			 const struct adftool_statement *statement,
			 uint32_t * id);

struct adftool_quads
{
  hid_t dataset;
  hid_t nextID;
};

static struct adftool_quads *
adftool_quads_alloc (void)
{
  struct adftool_quads *ret = malloc (sizeof (struct adftool_quads));
  if (ret != NULL)
    {
      ret->dataset = H5I_INVALID_HID;
      ret->nextID = H5I_INVALID_HID;
    }
  return ret;
}

static void
adftool_quads_free (struct adftool_quads *quads)
{
  if (quads != NULL)
    {
      H5Dclose (quads->dataset);
      H5Aclose (quads->nextID);
    }
  free (quads);
}

static int
adftool_quads_setup (struct adftool_quads *quads, hid_t file)
{
  int error = 0;
  H5Dclose (quads->dataset);
  H5Aclose (quads->nextID);
  quads->dataset = H5I_INVALID_HID;
  quads->nextID = H5I_INVALID_HID;
  quads->dataset = H5Dopen2 (file, "/data-description/quads", H5P_DEFAULT);
  if (quads->dataset == H5I_INVALID_HID)
    {
      hsize_t minimum_dimensions[] = { 0, 5 };
      hsize_t maximum_dimensions[] = { H5S_UNLIMITED, 5 };
      hsize_t chunk_dimensions[] = { 4096, 5 };
      hid_t fspace =
	H5Screate_simple (2, minimum_dimensions, maximum_dimensions);
      if (fspace == H5I_INVALID_HID)
	{
	  error = 1;
	  goto cleanup;
	}
      hid_t dataset_creation_properties = H5Pcreate (H5P_DATASET_CREATE);
      if (dataset_creation_properties == H5I_INVALID_HID)
	{
	  error = 1;
	  goto cleanup_fspace;
	}
      if (H5Pset_chunk (dataset_creation_properties, 2, chunk_dimensions) < 0)
	{
	  error = 1;
	  goto cleanup_dcpl;
	}
      hid_t link_creation_properties = H5Pcreate (H5P_LINK_CREATE);
      if (link_creation_properties == H5I_INVALID_HID)
	{
	  error = 1;
	  goto cleanup_dcpl;
	}
      if (H5Pset_create_intermediate_group (link_creation_properties, 1) < 0)
	{
	  error = 1;
	  goto cleanup_lcpl;
	}
      quads->dataset =
	H5Dcreate2 (file, "/data-description/quads", H5T_NATIVE_B64,
		    fspace, link_creation_properties,
		    dataset_creation_properties, H5P_DEFAULT);
      if (quads->dataset == H5I_INVALID_HID)
	{
	  error = 1;
	  goto cleanup_lcpl;
	}
    cleanup_lcpl:
      H5Pclose (link_creation_properties);
    cleanup_dcpl:
      H5Pclose (dataset_creation_properties);
    cleanup_fspace:
      H5Sclose (fspace);
    }
  quads->nextID = H5Aopen (quads->dataset, "nextID", H5P_DEFAULT);
  if (quads->nextID == H5I_INVALID_HID)
    {
      hid_t fspace = H5Screate (H5S_SCALAR);
      if (fspace == H5I_INVALID_HID)
	{
	  error = 1;
	  goto cleanup;
	}
      quads->nextID =
	H5Acreate2 (quads->dataset, "nextID", H5T_NATIVE_INT,
		    fspace, H5P_DEFAULT, H5P_DEFAULT);
      H5Sclose (fspace);
      if (quads->nextID == H5I_INVALID_HID)
	{
	  error = 1;
	  goto cleanup;
	}
      int zero = 0;
      if (H5Awrite (quads->nextID, H5T_NATIVE_INT, &zero) < 0)
	{
	  error = 1;
	  goto cleanup;
	}
    }
cleanup:
  if (error)
    {
      H5Dclose (quads->dataset);
      H5Aclose (quads->nextID);
      quads->dataset = H5I_INVALID_HID;
      quads->nextID = H5I_INVALID_HID;
    }
  return error;
}

static int
adftool_quads_update (struct adftool_quads *quads,
		      struct adftool_dictionary_index *dictionary,
		      uint32_t id, int read,
		      int (*update) (void *, const struct adftool_statement *,
				     struct adftool_statement *),
		      void *context)
{
  int error = 0;
  int next_id;
  if (H5Aread (quads->nextID, H5T_NATIVE_INT, &next_id) < 0)
    {
      error = 1;
      goto wrapup;
    }
  if (next_id < 0 || id >= (uint32_t) next_id)
    {
      error = 1;
      goto wrapup;
    }
  hid_t dataset_space = H5Dget_space (quads->dataset);
  if (dataset_space == H5I_INVALID_HID)
    {
      error = 1;
      goto wrapup;
    }
  int rank = H5Sget_simple_extent_ndims (dataset_space);
  if (rank != 2)
    {
      error = 1;
      goto clean_dataset_space;
    }
  hsize_t true_dims[2];
  int check_rank = H5Sget_simple_extent_dims (dataset_space, true_dims, NULL);
  if (check_rank < 0 || check_rank != rank)
    {
      error = 1;
      goto clean_dataset_space;
    }
  if (true_dims[1] != 5)
    {
      error = 1;
      goto clean_dataset_space;
    }
  hsize_t selection_start[2] = { 0, 0 };
  hsize_t selection_count[2] = { 1, 5 };
  selection_start[0] = id;
  hid_t selection_space = H5Screate_simple (2, true_dims, NULL);
  if (selection_space == H5I_INVALID_HID)
    {
      error = 1;
      goto clean_dataset_space;
    }
  if (H5Sselect_hyperslab
      (selection_space, H5S_SELECT_AND, selection_start, NULL,
       selection_count, NULL) < 0)
    {
      error = 1;
      goto clean_selection_space;
    }
  hsize_t memory_length = 5;
  uint64_t memory[5];
  hid_t memory_space = H5Screate_simple (1, &memory_length, NULL);
  if (memory_space == H5I_INVALID_HID)
    {
      error = 1;
      goto clean_selection_space;
    }
  if (read)
    {
      herr_t read_error = H5Dread (quads->dataset, H5T_NATIVE_B64,
				   memory_space, selection_space, H5P_DEFAULT,
				   memory);
      if (read_error < 0)
	{
	  error = 1;
	  goto clean_memory_space;
	}
    }
  struct adftool_term *subject = term_alloc ();
  struct adftool_term *predicate = term_alloc ();
  struct adftool_term *object = term_alloc ();
  struct adftool_term *graph = term_alloc ();
  struct adftool_statement *statement = statement_alloc ();
  struct adftool_statement *updated_statement = statement_alloc ();
  if (subject == NULL || predicate == NULL || object == NULL || graph == NULL
      || statement == NULL || updated_statement == NULL)
    {
      abort ();
    }
  if (read && memory[0] != ((uint64_t) (-1)))
    {
      if (term_decode (dictionary, memory[0], graph) != 0)
	{
	  error = 1;
	  goto clean_terms;
	}
      statement_set (statement, NULL, NULL, NULL, &graph, NULL);
    }
  else if (read)
    {
      struct adftool_term *no_graph = NULL;
      statement_set (statement, NULL, NULL, NULL, &no_graph, NULL);
    }
  if (read)
    {
      if ((term_decode (dictionary, memory[1], subject) != 0)
	  || (term_decode (dictionary, memory[2], predicate) != 0)
	  || (term_decode (dictionary, memory[3], object) != 0))
	{
	  error = 1;
	  goto clean_terms;
	}
      statement_set (statement, &subject, &predicate, &object, NULL,
		     &(memory[4]));
    }
  else
    {
      struct adftool_term *unset = NULL;
      uint64_t unset_date = ((uint64_t) (-1));
      statement_set (statement, &unset, &unset, &unset, &unset, &unset_date);
    }
  if (update (context, statement, updated_statement) != 0)
    {
      /* We don’t need to fail here. */
      error = 0;
      goto clean_terms;
    }
  const struct adftool_term *us, *up, *uo, *ug;
  statement_get (updated_statement, (struct adftool_term **) &us,
		 (struct adftool_term **) &up,
		 (struct adftool_term **) &uo,
		 (struct adftool_term **) &ug, &(memory[4]));
  if (ug == NULL)
    {
      /* The empty string is the default graph. */
      term_set_named (graph, "");
      ug = graph;
    }
  if (us == NULL || up == NULL || uo == NULL)
    {
      error = 1;
      goto clean_terms;
    }
  if ((term_encode (dictionary, us, &(memory[1])) != 0)
      || (term_encode (dictionary, up, &(memory[2])) != 0)
      || (term_encode (dictionary, uo, &(memory[3])) != 0)
      || (term_encode (dictionary, ug, &(memory[0])) != 0))
    {
      error = 1;
      goto clean_terms;
    }
  herr_t write_error = H5Dwrite (quads->dataset, H5T_NATIVE_B64,
				 memory_space, selection_space, H5P_DEFAULT,
				 memory);
  if (write_error)
    {
      abort ();
    }
clean_terms:
  statement_free (updated_statement);
  statement_free (statement);
  term_free (graph);
  term_free (object);
  term_free (predicate);
  term_free (subject);
clean_memory_space:
  H5Sclose (memory_space);
clean_selection_space:
  H5Sclose (selection_space);
clean_dataset_space:
  H5Sclose (dataset_space);
wrapup:
  return error;
}

struct adftool_quads_deletion_context
{
  uint64_t deletion_date;
};

static int
adftool_quads_deletion_updater (void *ctx,
				const struct adftool_statement *original,
				struct adftool_statement *updated)
{
  struct adftool_quads_deletion_context *context = ctx;
  statement_copy (updated, original);
  statement_set (updated, NULL, NULL, NULL, NULL, &(context->deletion_date));
  return 0;
}

struct adftool_quads_initialization_context
{
  const struct adftool_statement *reference;
};

static int
adftool_quads_initialization_updater (void *ctx,
				      const struct adftool_statement
				      *original,
				      struct adftool_statement *updated)
{
  (void) original;
  struct adftool_quads_initialization_context *context = ctx;
  statement_copy (updated, context->reference);
  return 0;
}

struct adftool_quads_noop_context
{
  struct adftool_statement *dest;
};

static int
adftool_quads_noop_updater (void *ctx,
			    const struct adftool_statement *original,
			    struct adftool_statement *updated)
{
  (void) updated;
  struct adftool_quads_noop_context *context = ctx;
  statement_copy (context->dest, original);
  /* 1 means don’t update the file. */
  return 1;
}

static int
quads_get (struct adftool_quads *quads,
	   struct adftool_dictionary_index *dictionary, uint32_t id,
	   struct adftool_statement *statement)
{
  struct adftool_quads_noop_context ctx;
  ctx.dest = statement;
  return adftool_quads_update (quads, dictionary, id, 1,
			       adftool_quads_noop_updater, &ctx);
}

static int
quads_delete (struct adftool_quads *quads,
	      struct adftool_dictionary_index *dictionary, uint32_t id,
	      uint64_t deletion_date)
{
  struct adftool_quads_deletion_context ctx;
  ctx.deletion_date = deletion_date;
  return adftool_quads_update (quads, dictionary, id, 1,
			       adftool_quads_deletion_updater, &ctx);
}

static int
adftool_quads_initialize (struct adftool_quads *quads,
			  struct adftool_dictionary_index *dictionary,
			  uint32_t id, const struct adftool_statement *value)
{
  struct adftool_quads_initialization_context ctx;
  ctx.reference = value;
  return adftool_quads_update (quads, dictionary, id, 0,
			       adftool_quads_initialization_updater, &ctx);
}

static int
quads_insert (struct adftool_quads *quads,
	      struct adftool_dictionary_index *dictionary,
	      const struct adftool_statement *statement, uint32_t * id)
{
  int next_id_value;
  hid_t dataset_space = H5Dget_space (quads->dataset);
  if (dataset_space == H5I_INVALID_HID)
    {
      return 1;
    }
  int rank = H5Sget_simple_extent_ndims (dataset_space);
  if (rank != 2)
    {
      H5Sclose (dataset_space);
      return 1;
    }
  hsize_t true_dims[2];
  int check_rank = H5Sget_simple_extent_dims (dataset_space, true_dims, NULL);
  if (check_rank < 0 || check_rank != rank)
    {
      H5Sclose (dataset_space);
      return 1;
    }
  H5Sclose (dataset_space);
  if (H5Aread (quads->nextID, H5T_NATIVE_INT, &next_id_value) < 0)
    {
      return 1;
    }
  if (next_id_value >= 0 && (uint32_t) next_id_value == true_dims[0])
    {
      /* We must grow the dataset. */
      true_dims[0] *= 2;
      if (true_dims[0] == 0)
	{
	  true_dims[0] = 1;
	}
      if (H5Dset_extent (quads->dataset, true_dims) < 0)
	{
	  fprintf (stderr, _("Could not extend the dataset.\n"));
	  abort ();
	}
    }
  *id = next_id_value;
  next_id_value += 1;
  if (H5Awrite (quads->nextID, H5T_NATIVE_INT, &next_id_value) < 0)
    {
      return 1;
    }
  if (adftool_quads_initialize (quads, dictionary, *id, statement) != 0)
    {
      /* Try to undo */
      next_id_value -= 1;
      if (H5Awrite (quads->nextID, H5T_NATIVE_INT, &next_id_value) < 0)
	{
	  abort ();
	}
      return 1;
    }
  return 0;
}

#endif /* not H_ADFTOOL_QUADS_INCLUDED */
