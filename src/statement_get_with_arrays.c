#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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

static void
check_n3 (const struct adftool_term *term, const char *expected_n3)
{
  char out[64];
  assert (strlen (expected_n3) + 1 <= sizeof (out));
  size_t n_out = adftool_term_to_n3 (term, 0, sizeof (out), out);
  assert (n_out < sizeof (out));
  out[sizeof (out) - 1] = '\0';	/* Unnecessary but still doesn’t
				   hurt */
  if (strcmp (out, expected_n3) != 0)
    {
      abort ();
    }
}

static void
check (const struct adftool_statement *statement,
       const char *subject_n3,
       const char *predicate_n3,
       const char *object_n3,
       const char *graph_n3,
       double deletion_date_high, double deletion_date_low)
{
  struct adftool_array_pointer *term_addresses =
    adftool_array_pointer_alloc (4);
  if (term_addresses == NULL)
    {
      abort ();
    }
  struct adftool_array_uint64_t *date_address =
    adftool_array_uint64_t_alloc (1);
  if (date_address == NULL)
    {
      abort ();
    }
  adftool_statement_get (statement,
			 (struct adftool_term **)
			 adftool_array_pointer_address (term_addresses, 0),
			 (struct adftool_term **)
			 adftool_array_pointer_address (term_addresses, 1),
			 (struct adftool_term **)
			 adftool_array_pointer_address (term_addresses, 2),
			 (struct adftool_term **)
			 adftool_array_pointer_address (term_addresses, 3),
			 (uint64_t *)
			 adftool_array_uint64_t_address (date_address, 0));
  if (subject_n3)
    {
      check_n3 (adftool_array_pointer_get (term_addresses, 0), subject_n3);
    }
  else
    {
      assert (adftool_array_pointer_get (term_addresses, 0) == NULL);
    }
  if (predicate_n3)
    {
      check_n3 (adftool_array_pointer_get (term_addresses, 1), predicate_n3);
    }
  else
    {
      assert (adftool_array_pointer_get (term_addresses, 1) == NULL);
    }
  if (object_n3)
    {
      check_n3 (adftool_array_pointer_get (term_addresses, 2), object_n3);
    }
  else
    {
      assert (adftool_array_pointer_get (term_addresses, 2) == NULL);
    }
  if (graph_n3)
    {
      check_n3 (adftool_array_pointer_get (term_addresses, 3), graph_n3);
    }
  else
    {
      assert (adftool_array_pointer_get (term_addresses, 3) == NULL);
    }
  assert (adftool_array_uint64_t_get_js_high (date_address, 0) ==
	  deletion_date_high);
  assert (adftool_array_uint64_t_get_js_low (date_address, 0) ==
	  deletion_date_low);
  adftool_array_uint64_t_free (date_address);
  adftool_array_pointer_free (term_addresses);
}

int
main (int argc, char *argv[])
{
  (void) argc;
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  textdomain (PACKAGE);
  struct adftool_term *triple[3];
  for (size_t i = 0; i < sizeof (triple) / sizeof (triple[0]); i++)
    {
      triple[i] = adftool_term_alloc ();
      if (triple[i] == NULL)
	{
	  abort ();
	}
      char letter = 'a' + i;
      char name[2] = { letter, '\0' };
      adftool_term_set_named (triple[i], name);
    }
  struct adftool_statement *statement = adftool_statement_alloc ();
  if (statement == NULL)
    {
      abort ();
    }
  adftool_statement_set (statement, &(triple[0]), &(triple[1]), &(triple[2]),
			 NULL, NULL);
  check (statement, "<a>", "<b>", "<c>", NULL, 4294967295, 4294967295);
  uint64_t deletion_date = 42;
  adftool_statement_set (statement, NULL, NULL, NULL, NULL, &deletion_date);
  check (statement, "<a>", "<b>", "<c>", NULL, 0, 42);
  adftool_statement_free (statement);
  for (size_t i = sizeof (triple) / sizeof (triple[0]); i-- > 0;)
    {
      adftool_term_free (triple[i]);
    }
  return 0;
}
