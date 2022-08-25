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

#ifdef HAVE_MPFR_H
#include <mpfr.h>
#endif /* HAVE_MPFR_H */

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
  if (adftool_file_open_data (file, 0, NULL) != 0)
    {
      abort ();
    }
  struct adftool_term *terms[3];
  for (size_t i = 0; i < sizeof (terms) / sizeof (terms[0]); i++)
    {
      terms[i] = adftool_term_alloc ();
      if (terms[i] == NULL)
	{
	  abort ();
	}
      char name[2] = "a";
      name[0] = 'a' + i;
      if (adftool_term_set_named (terms[i], name) != 0)
	{
	  abort ();
	}
    }
  struct adftool_statement *statement = adftool_statement_alloc ();
  if (statement == NULL)
    {
      abort ();
    }
  if ((adftool_statement_set_subject (statement, terms[0]) != 0)
      || (adftool_statement_set_predicate (statement, terms[1]) != 0)
      || (adftool_statement_set_object (statement, terms[2]) != 0))
    {
      abort ();
    }
  if (adftool_insert (file, statement) != 0)
    {
      abort ();
    }
  size_t file_length = adftool_file_get_data (file, 0, 0, NULL);
  uint8_t *file_data = malloc (file_length);
  if (file_data == NULL)
    {
      abort ();
    }
  if (adftool_file_get_data (file, 0, file_length, file_data) != file_length)
    {
      abort ();
    }
  adftool_statement_free (statement);
  for (size_t i = 0; i < sizeof (terms) / sizeof (terms[0]); i++)
    {
      adftool_term_free (terms[i]);
    }
  adftool_file_close (file);
  adftool_file_free (file);
  /* Now read the file… */
  file = adftool_file_alloc ();
  if (file == NULL)
    {
      abort ();
    }
  if (adftool_file_open_data (file, file_length, file_data) != 0)
    {
      abort ();
    }
  struct adftool_statement *pattern = adftool_statement_alloc ();
  if (pattern == NULL)
    {
      abort ();
    }
  struct adftool_results *results = adftool_results_alloc ();
  if (results == NULL)
    {
      abort ();
    }
  if (adftool_lookup (file, pattern, results) != 0)
    {
      abort ();
    }
  if (adftool_results_count (results) != 1)
    {
      abort ();
    }
  const struct adftool_statement *result = adftool_results_get (results, 0);
  for (size_t i = 0; i < sizeof (terms) / sizeof (terms[0]); i++)
    {
      terms[i] = adftool_term_alloc ();
    }
  int has_subject, has_predicate, has_object;
  if ((adftool_statement_get_subject (result, &has_subject, terms[0]) != 0)
      || (adftool_statement_get_predicate (result, &has_predicate, terms[1])
	  != 0)
      || (adftool_statement_get_object (result, &has_object, terms[2]) != 0))
    {
      abort ();
    }
  if (!has_subject || !has_predicate || !has_object)
    {
      abort ();
    }
  char *term_values[3];
  for (size_t i = 0; i < sizeof (terms) / sizeof (terms[0]); i++)
    {
      if (!adftool_term_is_named (terms[i]))
	{
	  abort ();
	}
      size_t length = adftool_term_value (terms[i], 0, 0, NULL);
      term_values[i] = malloc (length + 1);
      if (term_values[i] == NULL)
	{
	  abort ();
	}
      size_t check_length =
	adftool_term_value (terms[i], 0, length + 1, term_values[i]);
      assert (length == check_length);
    }
  if ((strcmp (term_values[0], "a") != 0)
      || (strcmp (term_values[1], "b") != 0)
      || (strcmp (term_values[2], "c") != 0))
    {
      abort ();
    }
  for (size_t i = 0; i < sizeof (terms) / sizeof (terms[0]); i++)
    {
      free (term_values[i]);
      adftool_term_free (terms[i]);
    }
  adftool_results_free (results);
  adftool_statement_free (pattern);
  adftool_file_close (file);
  adftool_file_free (file);
  free (file_data);
#ifdef HAVE_MPFR_FREE_CACHE
  mpfr_free_cache ();
#endif
  return 0;
}
