#ifndef H_ADFTOOL_STATEMENT_INCLUDED
# define H_ADFTOOL_STATEMENT_INCLUDED

# include <adftool.h>
# include <bplus.h>
# include <hdf5.h>

# include "term.h"

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

struct adftool_statement
{
  struct adftool_term *subject;
  struct adftool_term *predicate;
  struct adftool_term *object;
  struct adftool_term *graph;
  uint64_t deletion_date;
};

static struct adftool_statement *statement_alloc (void);
static void statement_free (struct adftool_statement *statement);

static void statement_set (struct adftool_statement *statement,
			   struct adftool_term **subject,
			   struct adftool_term **predicate,
			   struct adftool_term **object,
			   struct adftool_term **graph,
			   const uint64_t * deletion_date);
static void statement_get (const struct adftool_statement *statement,
			   struct adftool_term **subject,
			   struct adftool_term **predicate,
			   struct adftool_term **object,
			   struct adftool_term **graph,
			   uint64_t * deletion_date);

static int statement_compare (const struct adftool_statement *reference,
			      const struct adftool_statement *other,
			      const char *order);

static void statement_copy (struct adftool_statement *dest,
			    const struct adftool_statement *source);

struct adftool_statement *
statement_alloc (void)
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

static void
statement_free (struct adftool_statement *statement)
{
  if (statement != NULL)
    {
      term_free (statement->subject);
      term_free (statement->predicate);
      term_free (statement->object);
      term_free (statement->graph);
    }
  free (statement);
}

static void
adftool_statement_setter (struct adftool_term **dest,
			  struct adftool_term **source)
{
  if (source)
    {
      /* We want to change it. */
      if (*dest)
	{
	  term_free (*dest);
	  *dest = NULL;
	}
      if (*source)
	{
	  /* We want to change to a meaningful value. */
	  *dest = term_alloc ();
	  if (*dest == NULL)
	    {
	      /* FIXME: share the memory of *source, with reference
	         counting of the literal value? */
	      abort ();
	    }
	  term_copy (*dest, *source);
	}
      else
	{
	  /* We want to unset *dest. Nothing to do now. */
	}
    }
}

static void
statement_set (struct adftool_statement *statement,
	       struct adftool_term **subject,
	       struct adftool_term **predicate,
	       struct adftool_term **object,
	       struct adftool_term **graph, const uint64_t * deletion_date)
{
  adftool_statement_setter (&(statement->subject), subject);
  adftool_statement_setter (&(statement->predicate), predicate);
  adftool_statement_setter (&(statement->object), object);
  adftool_statement_setter (&(statement->graph), graph);
  if (deletion_date)
    {
      statement->deletion_date = *deletion_date;
    }
}

static void
adftool_statement_getter (const struct adftool_term *source,
			  struct adftool_term **dest)
{
  if (dest)
    {
      *dest = (struct adftool_term *) source;
    }
}

static void
statement_get (const struct adftool_statement *statement,
	       struct adftool_term **subject, struct adftool_term **predicate,
	       struct adftool_term **object, struct adftool_term **graph,
	       uint64_t * deletion_date)
{
  adftool_statement_getter (statement->subject, subject);
  adftool_statement_getter (statement->predicate, predicate);
  adftool_statement_getter (statement->object, object);
  adftool_statement_getter (statement->graph, graph);
  if (deletion_date)
    {
      *deletion_date = statement->deletion_date;
    }
}

static int
adftool_statement_compare_one (const struct adftool_statement *a,
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
      return term_compare (ta, tb);
    }
  return 0;
}

static int
statement_compare (const struct adftool_statement *reference,
		   const struct adftool_statement *other, const char *order)
{
  int result = 0;
  for (size_t i = 0; i < strlen (order); i++)
    {
      if (result == 0)
	{
	  result = adftool_statement_compare_one (reference, other, order[i]);
	}
    }
  return result;
}

static void
statement_copy (struct adftool_statement *dest,
		const struct adftool_statement *source)
{
  const struct adftool_term *subject;
  const struct adftool_term *predicate;
  const struct adftool_term *object;
  const struct adftool_term *graph;
  uint64_t deletion_date;
  statement_get (source, (struct adftool_term **) &subject,
		 (struct adftool_term **) &predicate,
		 (struct adftool_term **) &object,
		 (struct adftool_term **) &graph, &deletion_date);
  statement_set (dest, (struct adftool_term **) &subject,
		 (struct adftool_term **) &predicate,
		 (struct adftool_term **) &object,
		 (struct adftool_term **) &graph, &deletion_date);
}

#endif /* not H_ADFTOOL_STATEMENT_INCLUDED */
