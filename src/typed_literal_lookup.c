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
  remove ("typed_literal_lookup.adf");
  struct adftool_file *file =
    adftool_file_open ("typed_literal_lookup.adf", 1);
  if (file == NULL)
    {
      abort ();
    }
  struct adftool_term *subject = adftool_term_alloc ();
  if (subject == NULL)
    {
      abort ();
    }
  adftool_term_set_named (subject, "");
  static const char *predicate = "https://example.com/test";
  /* Fill the file with data */
  struct adftool_term *integer_object = adftool_term_alloc ();
  if (integer_object == NULL)
    {
      abort ();
    }
  adftool_term_set_integer (integer_object, 42);
  struct adftool_term *double_object = adftool_term_alloc ();
  if (double_object == NULL)
    {
      abort ();
    }
  adftool_term_set_double (double_object, 49.3);
  struct adftool_term *double_integer_object = adftool_term_alloc ();
  if (double_integer_object == NULL)
    {
      abort ();
    }
  adftool_term_set_double (double_integer_object, 69);
  struct adftool_term *date_object = adftool_term_alloc ();
  if (date_object == NULL)
    {
      abort ();
    }
  adftool_term_set_literal (date_object, "2022-02-09T10:14:32.12345",
			    "http://www.w3.org/2001/XMLSchema#dateTime",
			    NULL);
  struct adftool_term *plain_string_object = adftool_term_alloc ();
  if (plain_string_object == NULL)
    {
      abort ();
    }
  adftool_term_set_literal (plain_string_object, "foo", NULL, NULL);
  struct adftool_term *langstring_object = adftool_term_alloc ();
  if (langstring_object == NULL)
    {
      abort ();
    }
  adftool_term_set_literal (langstring_object, "hello, world!", NULL,
			    "en-us");
  struct adftool_term *objects_to_add[] = {
    integer_object,
    double_object,
    double_integer_object,
    date_object,
    plain_string_object,
    langstring_object
  };
  for (size_t i = 0; i < sizeof (objects_to_add) / sizeof (objects_to_add[0]);
       i++)
    {
      struct adftool_statement *statement = adftool_statement_alloc ();
      if (statement == NULL)
	{
	  abort ();
	}
      struct adftool_term *p = adftool_term_alloc ();
      if (p == NULL)
	{
	  abort ();
	}
      adftool_term_set_named (p, predicate);
      adftool_statement_set (statement, &subject, &p, &(objects_to_add[i]),
			     NULL, NULL);
      adftool_insert (file, statement);
      adftool_term_free (p);
      adftool_statement_free (statement);
    }
  long integer_value[2];
  double double_value[3];
  struct timespec date_storage[1];
  struct timespec *date_value[1];
  for (size_t i = 0; i < sizeof (date_storage) / sizeof (date_storage[0]);
       i++)
    {
      date_value[i] = &(date_storage[i]);
    }
  char storage[256];
  size_t max_storage = sizeof (storage);
  size_t storage_required;
  size_t string_value_lengths[2];
  char *string_value[2];
  size_t string_langtag_lengths[2];
  char *string_langtags[2];
  size_t n_integer_values =
    adftool_lookup_integer (file, subject, predicate, 0,
			    sizeof (integer_value) /
			    sizeof (integer_value[0]), integer_value);
  size_t n_double_values = adftool_lookup_double (file, subject, predicate, 0,
						  sizeof (double_value) /
						  sizeof (double_value[0]),
						  double_value);
  size_t n_date_values = adftool_lookup_date (file, subject, predicate, 0,
					      sizeof (date_value) /
					      sizeof (date_value[0]),
					      date_value);
  size_t n_string_values =
    adftool_lookup_string (file, subject, predicate, &storage_required,
			   max_storage, storage, 0,
			   sizeof (string_value) / sizeof (string_value[0]),
			   string_langtag_lengths, string_langtags,
			   string_value_lengths, string_value);
  assert (n_integer_values == 2);
  assert (n_double_values == 3);
  assert (n_date_values == 1);
  assert (n_string_values == 2);
  assert (integer_value[0] == 42);
  assert (integer_value[1] == 69);
  double double_value_0_error = double_value[0] - 49.3;
  if (double_value_0_error < 0)
    {
      double_value_0_error = -double_value_0_error;
    }
  assert (double_value_0_error < 1e-6);
  assert (double_value[1] == 42);
  assert (double_value[2] == 69);
  assert (date_value[0]->tv_sec == 1644401672);
  assert (date_value[0]->tv_nsec == 123450000);
  /* Strings: foo with no langtag, then hello, world! with en-us. Or
     is it the other order? */
  assert (string_value_lengths[0] == 3);
  assert (string_value_lengths[1] == 13);
  assert (STREQ (string_value[0], "foo"));
  assert (STREQ (string_value[1], "hello, world!"));
  assert (string_langtag_lengths[0] == 0);
  assert (string_langtag_lengths[1] == 5);
  assert (string_langtags[0] == NULL);
  assert (STREQ (string_langtags[1], "en-us"));
  adftool_term_free (langstring_object);
  adftool_term_free (plain_string_object);
  adftool_term_free (date_object);
  adftool_term_free (double_integer_object);
  adftool_term_free (double_object);
  adftool_term_free (integer_object);
  adftool_term_free (subject);
  adftool_file_close (file);
  remove ("typed_literal_lookup.adf");
  return 0;
}
