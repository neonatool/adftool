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
