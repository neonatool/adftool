#include <adftool_private.h>

static size_t
choose_index (const struct adftool_statement *pattern)
{
  /* See file.c to know the orders. */
#define INDEX_GSPO 0
#define INDEX_GPOS 1
#define INDEX_GOSP 2
#define INDEX_SPOG 3
#define INDEX_POSG 4
#define INDEX_OSPG 5
  size_t relevant_index = INDEX_SPOG;
  int can_use_spo = 1;
  int can_use_pos = 1;
  int can_use_osp = 1;
  if ((pattern->subject == NULL
       && (pattern->predicate != NULL || pattern->object != NULL))
      || (pattern->predicate == NULL && pattern->object != NULL))
    {
      can_use_spo = 0;
    }
  if ((pattern->predicate == NULL
       && (pattern->object != NULL || pattern->subject != NULL))
      || (pattern->object == NULL && pattern->subject != NULL))
    {
      can_use_pos = 0;
    }
  if ((pattern->object == NULL
       && (pattern->subject != NULL || pattern->predicate != NULL))
      || (pattern->subject == NULL && pattern->predicate != NULL))
    {
      can_use_osp = 0;
    }
  if (pattern->graph == NULL)
    {
      if (can_use_spo)
	{
	  relevant_index = INDEX_SPOG;
	}
      else if (can_use_pos)
	{
	  relevant_index = INDEX_POSG;
	}
      else if (can_use_osp)
	{
	  relevant_index = INDEX_OSPG;
	}
      else
	{
	  assert (0);
	}
    }
  else
    {
      if (can_use_spo)
	{
	  relevant_index = INDEX_GSPO;
	}
      else if (can_use_pos)
	{
	  relevant_index = INDEX_GPOS;
	}
      else if (can_use_osp)
	{
	  relevant_index = INDEX_GOSP;
	}
      else
	{
	  assert (0);
	}
    }
  return relevant_index;
}

static int
get_matches (const struct adftool_file *file,
	     const struct adftool_statement *pattern,
	     size_t *n_rows, uint32_t ** rows)
{
  size_t relevant_index = choose_index (pattern);
  const struct bplus *bplus =
    &(file->data_description.indices[relevant_index].bplus);
  struct adftool_bplus_key key;
  key.type = KEY_UNKNOWN;
  key.arg.unknown = (void *) pattern;
  int error = bplus_lookup (&key, (struct bplus *) bplus, 0, 0, n_rows, NULL);
  if (error)
    {
      return 1;
    }
  *rows = malloc (*n_rows * sizeof (uint32_t));
  if (*rows == NULL)
    {
      return 1;
    }
  size_t n_rows_check;
  error =
    bplus_lookup (&key, (struct bplus *) bplus, 0, *n_rows, &n_rows_check,
		  *rows);
  if (error || n_rows_check != *n_rows)
    {
      free (*rows);
      return 1;
    }
  return 0;
}

int
adftool_lookup (const struct adftool_file *file,
		const struct adftool_statement *pattern,
		size_t start, size_t max, size_t *n_results,
		struct adftool_statement **results)
{
  size_t n_rows;
  uint32_t *rows;
  if (get_matches (file, pattern, &n_rows, &rows) != 0)
    {
      return 1;
    }
  for (size_t i = start; i < n_rows && i - start < max; i++)
    {
      if (adftool_quads_get (file, rows[i], results[i]) != 0)
	{
	  free (rows);
	  return 1;
	}
    }
  *n_results = n_rows;
  free (rows);
  return 0;
}

size_t
adftool_lookup_objects (const struct adftool_file *file,
			const struct adftool_term *subject,
			const char *predicate, size_t start, size_t max,
			struct adftool_term **objects)
{
  struct adftool_term p = {.type = TERM_NAMED,.str1 =
      (char *) predicate,.str2 = NULL };
  struct adftool_statement pattern = {.subject =
      (struct adftool_term *) subject,.predicate = &p,.object = NULL,.graph =
      NULL,.deletion_date = ((uint64_t) (-1)) };
  struct adftool_statement *s = adftool_statement_alloc ();
  if (s == NULL)
    {
      abort ();
    }
  size_t n_rows;
  uint32_t *rows;
  if (get_matches (file, &pattern, &n_rows, &rows) != 0)
    {
      adftool_statement_free (s);
      return 0;
    }
  size_t n_live = 0;
  for (size_t i = 0; i < n_rows; i++)
    {
      if (adftool_quads_get (file, rows[i], s) == 0)
	{
	  if (s->deletion_date == ((uint64_t) (-1)))
	    {
	      if (n_live >= start && n_live < start + max)
		{
		  adftool_term_copy (objects[n_live], s->object);
		}
	      n_live++;
	    }
	}
    }
  free (rows);
  adftool_statement_free (s);
  return n_live;
}

size_t
adftool_lookup_subjects (const struct adftool_file *file,
			 const struct adftool_term *object,
			 const char *predicate, size_t start, size_t max,
			 struct adftool_term **subjects)
{
  struct adftool_term p = {.type = TERM_NAMED,.str1 =
      (char *) predicate,.str2 = NULL };
  struct adftool_statement pattern = {.subject = NULL,.predicate =
      &p,.object = (struct adftool_term *) object,.graph =
      NULL,.deletion_date = ((uint64_t) (-1)) };
  struct adftool_statement *s = adftool_statement_alloc ();
  if (s == NULL)
    {
      abort ();
    }
  size_t n_rows;
  uint32_t *rows;
  if (get_matches (file, &pattern, &n_rows, &rows) != 0)
    {
      adftool_statement_free (s);
      return 0;
    }
  size_t n_live = 0;
  for (size_t i = 0; i < n_rows; i++)
    {
      if (adftool_quads_get (file, rows[i], s) == 0)
	{
	  if (s->deletion_date == ((uint64_t) (-1)))
	    {
	      if (n_live >= start && n_live < start + max)
		{
		  adftool_term_copy (subjects[n_live], s->subject);
		}
	      n_live++;
	    }
	}
    }
  free (rows);
  adftool_statement_free (s);
  return n_live;
}

int
adftool_delete (struct adftool_file *file,
		const struct adftool_statement *pattern,
		uint64_t deletion_date)
{
  size_t n_rows;
  uint32_t *rows;
  if (get_matches (file, pattern, &n_rows, &rows) != 0)
    {
      return 1;
    }
  for (size_t i = 0; i < n_rows; i++)
    {
      if (adftool_quads_delete (file, rows[i], deletion_date) != 0)
	{
	  free (rows);
	  return 1;
	}
    }
  free (rows);
  return 0;
}

int
adftool_insert (struct adftool_file *file,
		const struct adftool_statement *statement)
{
  uint32_t id;
  if (adftool_quads_insert (file, statement, &id) != 0)
    {
      return 1;
    }
  for (size_t i = 0; i < 6; i++)
    {
      if (bplus_insert (id, id, &(file->data_description.indices[i].bplus)) !=
	  0)
	{
	  return 1;
	}
    }
  return 0;
}
