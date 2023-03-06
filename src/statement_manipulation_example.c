#include <config.h>

#include <attribute.h>
#include <adftool.h>
#include <hdf5.h>

#include <stdio.h>
#include <stdlib.h>
#include "gettext.h"
#include "relocatable.h"
#include "progname.h"
#include <locale.h>

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
  remove ("statement_example.adf");
  struct adftool_file *file = adftool_file_open ("statement_example.adf", 1);
  if (file == NULL)
    {
      abort ();
    }
  struct adftool_statement *statement = adftool_statement_alloc ();
  if (statement == NULL)
    {
      abort ();
    }
  struct adftool_term *term_a = adftool_term_alloc ();
  struct adftool_term *term_b = adftool_term_alloc ();
  struct adftool_term *term_c = adftool_term_alloc ();
  if (term_a == NULL || term_b == NULL || term_c == NULL)
    {
      abort ();
    }
  adftool_term_set_named (term_a, "a");
  adftool_term_set_named (term_b, "b");
  adftool_term_set_named (term_c, "c");
  adftool_statement_set (statement, &term_a, &term_b, &term_c, NULL, NULL);
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
  uint64_t deletion_date;
  adftool_statement_get (statement_zero, NULL, NULL, NULL, NULL,
			 &deletion_date);
  const int statement_zero_deleted = (deletion_date != ((uint64_t) (-1)));
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
  adftool_term_free (term_c);
  adftool_term_free (term_b);
  adftool_term_free (term_a);
  adftool_statement_free (statement);
  adftool_file_close (file);
  return 0;
}
