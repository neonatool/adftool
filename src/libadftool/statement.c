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

static int
copy_to (struct adftool_term **dest, const struct adftool_term *source)
{
  if (*dest && source == NULL)
    {
      adftool_term_free (*dest);
      *dest = NULL;
    }
  else if (*dest == NULL && source)
    {
      *dest = adftool_term_alloc ();
      if (*dest == NULL)
	{
	  return 1;
	}
    }
  if (source)
    {
      return adftool_term_copy (*dest, source);
    }
  return 0;
}

int
adftool_statement_set_subject (struct adftool_statement *statement,
			       const struct adftool_term *subject)
{
  return copy_to (&(statement->subject), subject);
}

int
adftool_statement_set_predicate (struct adftool_statement *statement,
				 const struct adftool_term *predicate)
{
  return copy_to (&(statement->predicate), predicate);
}

int
adftool_statement_set_object (struct adftool_statement *statement,
			      const struct adftool_term *object)
{
  return copy_to (&(statement->object), object);
}

int
adftool_statement_set_graph (struct adftool_statement *statement,
			     const struct adftool_term *graph)
{
  return copy_to (&(statement->graph), graph);
}

int
adftool_statement_set_deletion_date (struct adftool_statement *statement,
				     uint64_t deletion_date)
{
  statement->deletion_date = deletion_date;
  return 0;
}

int
adftool_statement_get_subject (const struct adftool_statement *statement,
			       int *has_subject, struct adftool_term *subject)
{
  *has_subject = (statement->subject != NULL);
  if (statement->subject)
    {
      return adftool_term_copy (subject, statement->subject);
    }
  return 0;
}

int
adftool_statement_get_predicate (const struct adftool_statement *statement,
				 int *has_predicate,
				 struct adftool_term *predicate)
{
  *has_predicate = (statement->predicate != NULL);
  if (statement->predicate)
    {
      return adftool_term_copy (predicate, statement->predicate);
    }
  return 0;
}

int
adftool_statement_get_object (const struct adftool_statement *statement,
			      int *has_object, struct adftool_term *object)
{
  *has_object = (statement->object != NULL);
  if (statement->object)
    {
      return adftool_term_copy (object, statement->object);
    }
  return 0;
}

int
adftool_statement_get_graph (const struct adftool_statement *statement,
			     int *has_graph, struct adftool_term *graph)
{
  *has_graph = (statement->graph != NULL);
  if (statement->graph)
    {
      return adftool_term_copy (graph, statement->graph);
    }
  return 0;
}

int
adftool_statement_get_deletion_date (const struct adftool_statement
				     *statement, int *has_date,
				     uint64_t * date)
{
  *has_date = (statement->deletion_date != (uint64_t) (-1));
  if (statement->deletion_date != (uint64_t) (-1))
    {
      *date = statement->deletion_date;
    }
  return 0;
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
      error = 1;
      goto clean_terms;
    }
  if (read && memory[0] != ((uint64_t) (-1)))
    {
      if (adftool_term_decode (file, memory[0], graph) != 0)
	{
	  error = 1;
	  goto clean_terms;
	}
      if (adftool_statement_set_graph (statement, graph) != 0)
	{
	  error = 1;
	  goto clean_terms;
	}
    }
  else
    {
      if (adftool_statement_set_graph (statement, NULL) != 0)
	{
	  error = 1;
	  goto clean_terms;
	}
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
      if ((adftool_statement_set_subject (statement, subject) != 0)
	  || (adftool_statement_set_predicate (statement, predicate) != 0)
	  || (adftool_statement_set_object (statement, object) != 0))
	{
	  error = 1;
	  goto clean_terms;
	}
      if (adftool_statement_set_deletion_date (statement, memory[4]) != 0)
	{
	  error = 1;
	  goto clean_terms;
	}
    }
  else
    {
      if ((adftool_statement_set_subject (statement, NULL) != 0)
	  || (adftool_statement_set_predicate (statement, NULL) != 0)
	  || (adftool_statement_set_object (statement, NULL) != 0)
	  || (adftool_statement_set_deletion_date (statement, (uint64_t) (-1))
	      != 0))
	{
	  error = 1;
	  goto clean_terms;
	}
    }
  if (update (context, statement, updated_statement) != 0)
    {
      /* We don’t need to fail here. */
      error = 0;
      goto clean_terms;
    }
  int has_subject, has_predicate, has_object, has_graph, has_deletion_date;
  if ((adftool_statement_get_subject
       (updated_statement, &has_subject, subject) != 0)
      ||
      (adftool_statement_get_predicate
       (updated_statement, &has_predicate, predicate) != 0)
      ||
      (adftool_statement_get_object (updated_statement, &has_object, object)
       != 0)
      || (adftool_statement_get_graph (updated_statement, &has_graph, graph)
	  != 0)
      ||
      (adftool_statement_get_deletion_date
       (updated_statement, &has_deletion_date, &(memory[4])) != 0))
    {
      error = 1;
      goto clean_terms;
    }
  if (has_graph)
    {
      if (adftool_term_encode (file, graph, &(memory[0])) != 0)
	{
	  error = 1;
	  goto clean_terms;
	}
    }
  else
    {
      memory[0] = ((uint64_t) (-1));
    }
  if (!has_subject || !has_predicate || !has_object)
    {
      error = 1;
      goto clean_terms;
    }
  if ((adftool_term_encode (file, subject, &(memory[1])) != 0)
      || (adftool_term_encode (file, predicate, &(memory[2])) != 0)
      || (adftool_term_encode (file, object, &(memory[3])) != 0))
    {
      error = 1;
      goto clean_terms;
    }
  if (!has_deletion_date)
    {
      memory[4] = ((uint64_t) (-1));
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
  int error;
  uint64_t deletion_date;
};

struct term_handler
{
  int (*get) (const struct adftool_statement *, int *, struct adftool_term *);
  int (*set) (struct adftool_statement *, const struct adftool_term *);
};

static int
deletion_updater (void *ctx, const struct adftool_statement *original,
		  struct adftool_statement *updated)
{
  struct deletion_context *context = ctx;
  context->error = 0;
  struct adftool_term *term = adftool_term_alloc ();
  int term_set;
  if (term == NULL)
    {
      context->error = 1;
      goto wrapup;
    }
  struct term_handler handlers[4];
  handlers[0].get = adftool_statement_get_subject;
  handlers[0].set = adftool_statement_set_subject;
  handlers[1].get = adftool_statement_get_predicate;
  handlers[1].set = adftool_statement_set_predicate;
  handlers[2].get = adftool_statement_get_object;
  handlers[2].set = adftool_statement_set_object;
  handlers[3].get = adftool_statement_get_graph;
  handlers[3].set = adftool_statement_set_graph;
  for (size_t i = 0; i < 4; i++)
    {
      if (handlers[i].get (original, &term_set, term) != 0)
	{
	  context->error = 1;
	  goto clean_term;
	}
      if (term_set)
	{
	  if (handlers[i].set (updated, term) != 0)
	    {
	      context->error = 1;
	      goto clean_term;
	    }
	}
      else
	{
	  if (handlers[i].set (updated, NULL) != 0)
	    {
	      context->error = 1;
	      goto clean_term;
	    }
	}
    }
  if (adftool_statement_set_deletion_date (updated, context->deletion_date) !=
      0)
    {
      context->error = 1;
      goto clean_term;
    }
clean_term:
  adftool_term_free (term);
wrapup:
  return context->error;
}

struct initialization_context
{
  int error;
  const struct adftool_statement *reference;
};

static int
initialization_updater (void *ctx, const struct adftool_statement *original,
			struct adftool_statement *updated)
{
  (void) original;
  struct initialization_context *context = ctx;
  context->error = 0;
  struct deletion_context del;
  del.error = 0;
  int reference_has_deletion_date = 0;
  if (adftool_statement_get_deletion_date
      (context->reference, &reference_has_deletion_date,
       &(del.deletion_date)) != 0)
    {
      context->error = 1;
      goto wrapup;
    }
  if (!reference_has_deletion_date)
    {
      del.deletion_date = ((uint64_t) (-1));
    }
  /* The hack there is to use the deletion_updater callback because it
     copies statements. */
  context->error = deletion_updater (&del, context->reference, updated);
  assert (context->error == del.error);
wrapup:
  return context->error;
}

struct noop_context
{
  int error;
  struct adftool_statement *dest;
};

static int
noop_updater (void *ctx, const struct adftool_statement *original,
	      struct adftool_statement *updated)
{
  (void) updated;
  struct noop_context *context = ctx;
  context->error = 0;
  struct initialization_context init;
  init.error = 0;
  init.reference = original;
  context->error = initialization_updater (&init, NULL, context->dest);
  /* 1 means don’t update the file. */
  return 1;
}

int
adftool_quads_get (const struct adftool_file *file, uint32_t id,
		   struct adftool_statement *statement)
{
  struct noop_context ctx;
  ctx.error = 0;
  ctx.dest = statement;
  int global_error =
    update_quad ((struct adftool_file *) file, id, 1, noop_updater, &ctx);
  return global_error || ctx.error;
}

int
adftool_quads_delete (struct adftool_file *file, uint32_t id,
		      uint64_t deletion_date)
{
  struct deletion_context ctx;
  ctx.error = 0;
  ctx.deletion_date = deletion_date;
  int global_error = update_quad (file, id, 1, deletion_updater, &ctx);
  return global_error || ctx.error;
}

static int
initialize_quad (struct adftool_file *file, uint32_t id,
		 const struct adftool_statement *value)
{
  struct initialization_context ctx;
  ctx.error = 0;
  ctx.reference = value;
  int global_error = update_quad (file, id, 0, initialization_updater, &ctx);
  return global_error || ctx.error;
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
