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
#include <assert.h>

#define _(String) gettext(String)
#define N_(String) (String)

#define STREQ(s1, s2) (strcmp ((s1), (s2)) == 0)
#define STRNEQ(s1, s2) (strcmp ((s1), (s2)) != 0)

int
main (int argc, char *argv[])
{
  (void) argc;
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);
  struct adftool_file *file = adftool_file_open_data (0, NULL);
  if (file == NULL)
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
      adftool_term_set_named (terms[i], name);
    }
  struct adftool_statement *statement = adftool_statement_alloc ();
  if (statement == NULL)
    {
      abort ();
    }
  adftool_statement_set (statement, &(terms[0]), &(terms[1]), &(terms[2]),
			 NULL, NULL);
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
  /* Now read the file… */
  file = adftool_file_open_data (file_length, file_data);
  if (file == NULL)
    {
      abort ();
    }
  struct adftool_statement *pattern = adftool_statement_alloc ();
  if (pattern == NULL)
    {
      abort ();
    }
  struct adftool_statement *result = adftool_statement_alloc ();
  size_t n_results;
  if (result == NULL)
    {
      abort ();
    }
  if (adftool_lookup (file, pattern, 0, 1, &n_results, &result) != 0)
    {
      abort ();
    }
  if (n_results != 1)
    {
      abort ();
    }
  for (size_t i = 0; i < sizeof (terms) / sizeof (terms[0]); i++)
    {
      terms[i] = adftool_term_alloc ();
    }
  const struct adftool_term *s, *p, *o;
  adftool_statement_get (result, (struct adftool_term **) &s,
			 (struct adftool_term **) &p,
			 (struct adftool_term **) &o, NULL, NULL);
  if (s == NULL || p == NULL || o == NULL)
    {
      abort ();
    }
  const struct adftool_term *result_terms[3];
  result_terms[0] = s;
  result_terms[1] = p;
  result_terms[2] = o;
  char *term_values[3];
  assert ((sizeof (result_terms) / sizeof (result_terms[0]))
	  == (sizeof (terms) / sizeof (terms[0])));
  assert ((sizeof (term_values) / sizeof (term_values[0]))
	  == (sizeof (terms) / sizeof (terms[0])));
  for (size_t i = 0; i < sizeof (terms) / sizeof (terms[0]); i++)
    {
      if (!adftool_term_is_named (result_terms[i]))
	{
	  abort ();
	}
      size_t length = adftool_term_value (result_terms[i], 0, 0, NULL);
      term_values[i] = malloc (length + 1);
      if (term_values[i] == NULL)
	{
	  abort ();
	}
      size_t check_length =
	adftool_term_value (result_terms[i], 0, length + 1, term_values[i]);
      assert (length == check_length);
    }
  if (STRNEQ (term_values[0], "a")
      || STRNEQ (term_values[1], "b") || STRNEQ (term_values[2], "c"))
    {
      abort ();
    }
  for (size_t i = 0; i < sizeof (terms) / sizeof (terms[0]); i++)
    {
      free (term_values[i]);
      adftool_term_free (terms[i]);
    }
  adftool_statement_free (result);
  adftool_statement_free (pattern);
  adftool_file_close (file);
  free (file_data);
  return 0;
}
