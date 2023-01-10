#include <config.h>

#include <adftool.h>

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
  struct adftool_statement *first_statement = adftool_statement_alloc ();
  struct adftool_statement *second_statement = adftool_statement_alloc ();
  struct adftool_statement *pattern = adftool_statement_alloc ();
  struct adftool_statement *result = adftool_statement_alloc ();
  struct adftool_term *letters[6];
  if (first_statement == NULL || second_statement == NULL
      || pattern == NULL || result == NULL)
    {
      abort ();
    }
  for (size_t i = 0; i < 6; i++)
    {
      char letter[2];
      letter[0] = 'a' + i;
      letter[1] = '\0';
      letters[i] = adftool_term_alloc ();
      if (letters[i] == NULL)
	{
	  abort ();
	}
      adftool_term_set_named (letters[i], letter);
    }
  adftool_statement_set (first_statement, &(letters[0]), &(letters[1]),
			 &(letters[2]), NULL, NULL);
  adftool_statement_set (second_statement, &(letters[3]), &(letters[4]),
			 &(letters[5]), NULL, NULL);
  struct adftool_file *file = adftool_file_open_data (0, NULL);
  if (file == NULL)
    {
      abort ();
    }
  if ((adftool_insert (file, first_statement) != 0)
      || (adftool_insert (file, second_statement) != 0))
    {
      abort ();
    }
  /* file contains: <a> <b> <c> . <d> <e> <f> . */
  /* pattern matches anything. */
  size_t n_results;
  int error = adftool_lookup (file, pattern, 1, 1, &n_results, &result);
  assert (error == 0);
  assert (n_results == 2);
  assert (adftool_statement_compare (result, second_statement, "SPOG") == 0);
  assert (adftool_statement_compare (second_statement, result, "SPOG") == 0);
  adftool_file_close (file);
  for (size_t i = 6; i-- > 0;)
    {
      adftool_term_free (letters[i]);
    }
  adftool_statement_free (result);
  adftool_statement_free (pattern);
  adftool_statement_free (second_statement);
  adftool_statement_free (first_statement);
  return 0;
}
