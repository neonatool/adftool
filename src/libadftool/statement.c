#include <adftool_private.h>
#include <adftool_bplus.h>

struct adftool_statement *
adftool_statement_alloc (void)
{
  struct adftool_statement *ret = malloc (sizeof (struct adftool_statement));
  if (ret != NULL)
    {
      ret->subject = NULL;
      ret->predicate = NULL;
      ret->object = NULL;
      ret->graph = NULL;
      ret->deletion_date = ((uint64_t) (-1));
    }
  return ret;
}

void
adftool_statement_free (struct adftool_statement *statement)
{
  if (statement != NULL)
    {
      adftool_term_free (statement->subject);
      adftool_term_free (statement->predicate);
      adftool_term_free (statement->object);
      adftool_term_free (statement->graph);
    }
  free (statement);
}

static void
setter (struct adftool_term **dest, struct adftool_term **source)
{
  if (source)
    {
      /* We want to change it. */
      if (*dest)
	{
	  adftool_term_free (*dest);
	  *dest = NULL;
	}
      if (*source)
	{
	  /* We want to change to a meaningful value. */
	  *dest = adftool_term_alloc ();
	  if (*dest == NULL)
	    {
	      /* FIXME: share the memory of *source, with reference
	         counting of the literal value? */
	      abort ();
	    }
	  adftool_term_copy (*dest, *source);
	}
      else
	{
	  /* We want to unset *dest. Nothing to do now. */
	}
    }
}

void
adftool_statement_set (struct adftool_statement *statement,
		       struct adftool_term **subject,
		       struct adftool_term **predicate,
		       struct adftool_term **object,
		       struct adftool_term **graph,
		       const uint64_t * deletion_date)
{
  setter (&(statement->subject), subject);
  setter (&(statement->predicate), predicate);
  setter (&(statement->object), object);
  setter (&(statement->graph), graph);
  if (deletion_date)
    {
      statement->deletion_date = *deletion_date;
    }
}

static void
getter (const struct adftool_term *source, struct adftool_term **dest)
{
  if (dest)
    {
      *dest = (struct adftool_term *) source;
    }
}

void
adftool_statement_get (const struct adftool_statement *statement,
		       struct adftool_term **subject,
		       struct adftool_term **predicate,
		       struct adftool_term **object,
		       struct adftool_term **graph, uint64_t * deletion_date)
{
  getter (statement->subject, subject);
  getter (statement->predicate, predicate);
  getter (statement->object, object);
  getter (statement->graph, graph);
  if (deletion_date)
    {
      *deletion_date = statement->deletion_date;
    }
}

static int
update_quad (struct adftool_file *file, uint32_t id, int read,
	     int (*update) (void *, const struct adftool_statement *,
			    struct adftool_statement *), void *context)
{
  int error = 0;
  int next_id;
  if (H5Aread (file->data_description.quads.nextid, H5T_NATIVE_INT, &next_id)
      < 0)
    {
      error = 1;
      goto wrapup;
    }
  if (next_id < 0 || id >= (uint32_t) next_id)
    {
      error = 1;
      goto wrapup;
    }
  hid_t dataset_space = H5Dget_space (file->data_description.quads.dataset);
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
      herr_t read_error =
	H5Dread (file->data_description.quads.dataset, H5T_NATIVE_B64,
		 memory_space, selection_space, H5P_DEFAULT, memory);
      if (read_error < 0)
	{
	  error = 1;
	  goto clean_memory_space;
	}
    }
  struct adftool_term *subject = adftool_term_alloc ();
  struct adftool_term *predicate = adftool_term_alloc ();
  struct adftool_term *object = adftool_term_alloc ();
  struct adftool_term *graph = adftool_term_alloc ();
  struct adftool_statement *statement = adftool_statement_alloc ();
  struct adftool_statement *updated_statement = adftool_statement_alloc ();
  if (subject == NULL || predicate == NULL || object == NULL || graph == NULL
      || statement == NULL || updated_statement == NULL)
    {
      abort ();
    }
  if (read && memory[0] != ((uint64_t) (-1)))
    {
      if (adftool_term_decode (file, memory[0], graph) != 0)
	{
	  error = 1;
	  goto clean_terms;
	}
      adftool_statement_set (statement, NULL, NULL, NULL, &graph, NULL);
    }
  else if (read)
    {
      struct adftool_term *no_graph = NULL;
      adftool_statement_set (statement, NULL, NULL, NULL, &no_graph, NULL);
    }
  if (read)
    {
      if ((adftool_term_decode (file, memory[1], subject) != 0)
	  || (adftool_term_decode (file, memory[2], predicate) != 0)
	  || (adftool_term_decode (file, memory[3], object) != 0))
	{
	  error = 1;
	  goto clean_terms;
	}
      adftool_statement_set (statement, &subject, &predicate, &object, NULL,
			     &(memory[4]));
    }
  else
    {
      struct adftool_term *unset = NULL;
      uint64_t unset_date = ((uint64_t) (-1));
      adftool_statement_set (statement, &unset, &unset, &unset, &unset,
			     &unset_date);
    }
  if (update (context, statement, updated_statement) != 0)
    {
      /* We don’t need to fail here. */
      error = 0;
      goto clean_terms;
    }
  const struct adftool_term *us, *up, *uo, *ug;
  adftool_statement_get (updated_statement, (struct adftool_term **) &us,
			 (struct adftool_term **) &up,
			 (struct adftool_term **) &uo,
			 (struct adftool_term **) &ug, &(memory[4]));
  if (ug == NULL)
    {
      /* The empty string is the default graph. */
      adftool_term_set_named (graph, "");
      ug = graph;
    }
  if (us == NULL || up == NULL || uo == NULL)
    {
      error = 1;
      goto clean_terms;
    }
  if ((adftool_term_encode (file, us, &(memory[1])) != 0)
      || (adftool_term_encode (file, up, &(memory[2])) != 0)
      || (adftool_term_encode (file, uo, &(memory[3])) != 0)
      || (adftool_term_encode (file, ug, &(memory[0])) != 0))
    {
      error = 1;
      goto clean_terms;
    }
  herr_t write_error =
    H5Dwrite (file->data_description.quads.dataset, H5T_NATIVE_B64,
	      memory_space, selection_space, H5P_DEFAULT, memory);
  if (write_error)
    {
      abort ();
    }
clean_terms:
  adftool_statement_free (updated_statement);
  adftool_statement_free (statement);
  adftool_term_free (graph);
  adftool_term_free (object);
  adftool_term_free (predicate);
  adftool_term_free (subject);
clean_memory_space:
  H5Sclose (memory_space);
clean_selection_space:
  H5Sclose (selection_space);
clean_dataset_space:
  H5Sclose (dataset_space);
wrapup:
  return error;
}

struct deletion_context
{
  uint64_t deletion_date;
};

static int
deletion_updater (void *ctx, const struct adftool_statement *original,
		  struct adftool_statement *updated)
{
  struct deletion_context *context = ctx;
  adftool_statement_copy (updated, original);
  adftool_statement_set (updated, NULL, NULL, NULL, NULL,
			 &(context->deletion_date));
  return 0;
}

struct initialization_context
{
  const struct adftool_statement *reference;
};

static int
initialization_updater (void *ctx, const struct adftool_statement *original,
			struct adftool_statement *updated)
{
  (void) original;
  struct initialization_context *context = ctx;
  adftool_statement_copy (updated, context->reference);
  return 0;
}

struct noop_context
{
  struct adftool_statement *dest;
};

static int
noop_updater (void *ctx, const struct adftool_statement *original,
	      struct adftool_statement *updated)
{
  (void) updated;
  struct noop_context *context = ctx;
  adftool_statement_copy (context->dest, original);
  /* 1 means don’t update the file. */
  return 1;
}

int
adftool_quads_get (const struct adftool_file *file, uint32_t id,
		   struct adftool_statement *statement)
{
  struct noop_context ctx;
  ctx.dest = statement;
  return update_quad ((struct adftool_file *) file, id, 1, noop_updater,
		      &ctx);
}

int
adftool_quads_delete (struct adftool_file *file, uint32_t id,
		      uint64_t deletion_date)
{
  struct deletion_context ctx;
  ctx.deletion_date = deletion_date;
  return update_quad (file, id, 1, deletion_updater, &ctx);
}

static int
initialize_quad (struct adftool_file *file, uint32_t id,
		 const struct adftool_statement *value)
{
  struct initialization_context ctx;
  ctx.reference = value;
  return update_quad (file, id, 0, initialization_updater, &ctx);
}

int
adftool_quads_insert (struct adftool_file *file,
		      const struct adftool_statement *statement,
		      uint32_t * id)
{
  int next_id_value;
  hid_t dataset_space = H5Dget_space (file->data_description.quads.dataset);
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
  if (H5Aread
      (file->data_description.quads.nextid, H5T_NATIVE_INT,
       &next_id_value) < 0)
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
      if (H5Dset_extent (file->data_description.quads.dataset, true_dims) < 0)
	{
	  fprintf (stderr, _("Could not extend the dataset.\n"));
	  abort ();
	}
    }
  *id = next_id_value;
  next_id_value += 1;
  if (H5Awrite
      (file->data_description.quads.nextid, H5T_NATIVE_INT,
       &next_id_value) < 0)
    {
      return 1;
    }
  if (initialize_quad (file, *id, statement) != 0)
    {
      /* Try to undo */
      next_id_value -= 1;
      if (H5Awrite
	  (file->data_description.quads.nextid, H5T_NATIVE_INT,
	   &next_id_value) < 0)
	{
	  abort ();
	}
      return 1;
    }
  return 0;
}

static int
statements_compare_one (const struct adftool_statement *a,
			const struct adftool_statement *b, char what)
{
  const struct adftool_term *ta;
  const struct adftool_term *tb;
  switch (what)
    {
    case 'S':
      ta = a->subject;
      tb = b->subject;
      break;
    case 'P':
      ta = a->predicate;
      tb = b->predicate;
      break;
    case 'O':
      ta = a->object;
      tb = b->object;
      break;
    case 'G':
      ta = a->graph;
      tb = b->graph;
      break;
    default:
      abort ();
    }
  if (ta != NULL && tb != NULL)
    {
      return adftool_term_compare (ta, tb);
    }
  return 0;
}

int
adftool_statement_compare (const struct adftool_statement *reference,
			   const struct adftool_statement *other,
			   const char *order)
{
  int result = 0;
  for (size_t i = 0; i < strlen (order); i++)
    {
      if (result == 0)
	{
	  result = statements_compare_one (reference, other, order[i]);
	}
    }
  return result;
}

void
adftool_statement_copy (struct adftool_statement *dest,
			const struct adftool_statement *source)
{
  const struct adftool_term *subject;
  const struct adftool_term *predicate;
  const struct adftool_term *object;
  const struct adftool_term *graph;
  uint64_t deletion_date;
  adftool_statement_get (source, (struct adftool_term **) &subject,
			 (struct adftool_term **) &predicate,
			 (struct adftool_term **) &object,
			 (struct adftool_term **) &graph, &deletion_date);
  adftool_statement_set (dest, (struct adftool_term **) &subject,
			 (struct adftool_term **) &predicate,
			 (struct adftool_term **) &object,
			 (struct adftool_term **) &graph, &deletion_date);
}
