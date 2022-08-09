#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <adftool.h>
#include <hdf5.h>

#include <stdio.h>
#include <stdlib.h>
#include "gettext.h"
#include "relocatable.h"
#include "progname.h"
#include <locale.h>
#include <assert.h>

#define _(String) gettext(String)
#define N_(String) (String)


int
main (int argc, char *argv[])
{
  (void) argc;
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);
  struct adftool_file *file = adftool_file_alloc ();
  if (file == NULL)
    {
      abort ();
    }
  remove ("statement_example.adf");
  int error = adftool_file_open (file, "statement_example.adf", 1);
  if (error)
    {
      abort ();
    }
  struct adftool_statement *statement = adftool_statement_alloc ();
  if (statement == NULL)
    {
      abort ();
    }
  struct adftool_term *term = adftool_term_alloc ();
  if (term == NULL)
    {
      abort ();
    }
  if (adftool_term_set_named (term, "a") != 0)
    {
      abort ();
    }
  if (adftool_statement_set_subject (statement, term) != 0)
    {
      abort ();
    }
  if (adftool_term_set_named (term, "b") != 0)
    {
      abort ();
    }
  if (adftool_statement_set_predicate (statement, term) != 0)
    {
      abort ();
    }
  if (adftool_term_set_named (term, "c") != 0)
    {
      abort ();
    }
  if (adftool_statement_set_object (statement, term) != 0)
    {
      abort ();
    }
  uint32_t statement_id = 42;
  if (adftool_quads_insert (file, statement, &statement_id) != 0)
    {
      abort ();
    }
  struct adftool_statement *statement_zero = adftool_statement_alloc ();
  if (statement_zero == NULL)
    {
      abort ();
    }
  if (statement_id != 0)
    {
      abort ();
    }
  if (adftool_quads_delete (file, 0, 0) != 0)
    {
      abort ();
    }
  if (adftool_quads_get (file, 0, statement_zero) != 0)
    {
      abort ();
    }
  int statement_zero_deleted;
  uint64_t deletion_date;
  if (adftool_statement_get_deletion_date
      (statement_zero, &statement_zero_deleted, &deletion_date) != 0)
    {
      abort ();
    }
  if (!statement_zero_deleted)
    {
      abort ();
    }
  if (deletion_date != 0)
    {
      abort ();
    }
  if (adftool_statement_compare (statement, statement_zero, "SPOG") != 0)
    {
      abort ();
    }
  adftool_statement_free (statement_zero);
  adftool_term_free (term);
  adftool_statement_free (statement);
  adftool_file_close (file);
  adftool_file_free (file);
  return 0;
}
