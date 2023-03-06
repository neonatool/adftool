#include <config.h>

#include <attribute.h>
#include <adftool.h>

#define STREQ(s1, s2) (strcmp ((s1), (s2)) == 0)
#define STRNEQ(s1, s2) (strcmp ((s1), (s2)) != 0)

#include "file.h"
#include "quads.h"
#include "statement.h"

struct adftool_statement *
adftool_statement_alloc (void)
{
  return statement_alloc ();
}

void
adftool_statement_free (struct adftool_statement *statement)
{
  statement_free (statement);
}

void
adftool_statement_set (struct adftool_statement *statement,
		       struct adftool_term **subject,
		       struct adftool_term **predicate,
		       struct adftool_term **object,
		       struct adftool_term **graph,
		       const uint64_t * deletion_date)
{
  statement_set (statement, subject, predicate, object, graph, deletion_date);
}

void
adftool_statement_get (const struct adftool_statement *statement,
		       struct adftool_term **subject,
		       struct adftool_term **predicate,
		       struct adftool_term **object,
		       struct adftool_term **graph, uint64_t * deletion_date)
{
  statement_get (statement, subject, predicate, object, graph, deletion_date);
}

int
adftool_quads_get (const struct adftool_file *file, uint32_t id,
		   struct adftool_statement *statement)
{
  return quads_get (file->quads, file->dictionary, id, statement);
}

int
adftool_quads_delete (struct adftool_file *file, uint32_t id,
		      uint64_t deletion_date)
{
  return quads_delete (file->quads, file->dictionary, id, deletion_date);
}

int
adftool_quads_insert (struct adftool_file *file,
		      const struct adftool_statement *statement,
		      uint32_t * id)
{
  return quads_insert (file->quads, file->dictionary, statement, id);
}

int
adftool_statement_compare (const struct adftool_statement *reference,
			   const struct adftool_statement *other,
			   const char *order)
{
  return statement_compare (reference, other, order);
}

void
adftool_statement_copy (struct adftool_statement *dest,
			const struct adftool_statement *source)
{
  statement_copy (dest, source);
}
